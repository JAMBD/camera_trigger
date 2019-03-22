#include <Adafruit_GFX.h>
#include <Adafruit_SharpMem.h>
#include <EEPROM.h>

// any pins can be used
#define SHARP_SCK 15
#define SHARP_MOSI 16
#define SHARP_SS 10

Adafruit_SharpMem display(SHARP_SCK, SHARP_MOSI, SHARP_SS);

#define BLACK 0
#define WHITE 1

unsigned long frame_delay_s = 10;
unsigned long down_time_s = 2;
int sel = -1;
int item = 0;
long next_frame_millis = 0;
long button_up_millis = 0;
int frames_taken = 0;
unsigned long prev_refresh = 0;
unsigned long frame_count = 0;

int prev_center = HIGH;
int prev_up = HIGH;
int prev_down = HIGH;
char string[64];

const int magic = 0xDAFF0D11;

struct data {
  int frame_delay_s;
  int down_time_s;
  int frames;
  int magic;
};

void pre_draw(int row){
  display.setTextSize(1);
  display.setCursor(2,12*(row+1)+1);
  if (item == row && sel == row){
    display.fillRect(0, 12*(row+1)-1, display.width(), 11, BLACK);
    display.setTextColor(WHITE,BLACK);
  }else{
    display.fillRect(0, 12*(row+1)-1, display.width(), 11, WHITE);
    if (item == row )
      display.drawRect(0, 12*(row+1)-1, display.width(), 11, BLACK);
    display.setTextColor(BLACK, WHITE);
  }
}

void setup() {
  // put your setup code here, to run once:
  data saved;
  EEPROM.get(0, saved);
  if(saved.magic != magic){
    saved.frame_delay_s = frame_delay_s;
    saved.down_time_s = down_time_s;
    saved.frames = frame_count;
    saved.magic = magic;
    EEPROM.put(0, saved);
  }else{
    frame_delay_s = saved.frame_delay_s;
    down_time_s = saved.down_time_s;
    frame_count = saved.frames;
  }
  Serial.begin(9600);
  display.begin();
  display.setRotation(2);
  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor(9,1);
  display.fillRect(0, 0, display.width(), 11, BLACK);
  display.drawLine(0, 10, display.width()-1, 10, WHITE);
  display.setTextColor(WHITE, BLACK);
  display.println("CamTimer v0.1");
  
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  digitalWrite(4, LOW);
  digitalWrite(5, LOW);
  
  pinMode(7, INPUT_PULLUP);
  pinMode(8, INPUT_PULLUP);
  pinMode(9, INPUT_PULLUP);
  
}

void loop() {

  if(millis() - prev_refresh > 100){

    pre_draw(0);
    if(next_frame_millis > 0){
      sprintf(string,"Trigger:[%c]", digitalRead(5)?' ':'#');
    }else{
      sprintf(string,"Trigger");
    }
    display.println(string);
    
    pre_draw(1);
    if(next_frame_millis > 0){
      unsigned long diff = (next_frame_millis - millis())/100;
      if (next_frame_millis < millis()){
        diff = 0;
      }
      int sec = diff / 10;
      int dsec = diff%10;
      sprintf(string,"Frame in %2d.%01ds", sec, dsec);
    }else{
      sprintf(string,"Frame delay:%ds", frame_delay_s);
    }
    display.println(string);
    
    pre_draw(2);
    if(next_frame_millis > 0){
      long diff = (button_up_millis - millis())/100;
      if (button_up_millis == -1 || button_up_millis < millis()){
        diff = 0;
      }
      int sec = diff / 10;
      int dsec = diff%10;
      sprintf(string,"Hold for %2d.%01ds", sec, dsec);
    }else{
      sprintf(string,"Down time:%ds", down_time_s);
    }
    display.println(string);

    pre_draw(3);
    if(next_frame_millis > 0){
      sprintf(string,"Took %3d of %3d", frames_taken, frame_count);
    }else{
      sprintf(string,"Frames: %d", frame_count);
    }
    display.println(string);
    
    pre_draw(4);
    display.println("Save defaults");

    if(sel == 4){
      data saved;
      saved.frame_delay_s = frame_delay_s;
      saved.down_time_s = down_time_s;
      saved.frames = frame_count;
      saved.magic = magic;
      EEPROM.put(0, saved);
      sel = -1;
    }
    
    prev_down = HIGH;
    prev_up = HIGH;
    prev_refresh = millis();
    display.refresh();
  }

  if (next_frame_millis > 0 ){
    if (next_frame_millis < millis()){
      frames_taken ++;
      button_up_millis = next_frame_millis + down_time_s * 1000;
      next_frame_millis += max(frame_delay_s, down_time_s)*1000;
      digitalWrite(5, HIGH);
    }
  }
  
  if (button_up_millis > 0 ){
    if (button_up_millis < millis()){
      if (frame_count > 0 && frames_taken >= frame_count){
        sel = -1;
        next_frame_millis = 0;
      }
      button_up_millis = -1;
      digitalWrite(5, LOW);
    }
  }
  int cur_center = digitalRead(8);
  if(!cur_center && prev_center){
    if (sel == -1){
      sel = item;
      if(item == 0){
        next_frame_millis = millis();
        button_up_millis = -1;
        frames_taken = 0;
      }
    }else{
      sel = -1;
      if(item == 0){
        next_frame_millis = 0;
      }
    }
  }
  prev_center = cur_center;

  int cur_up = digitalRead(7);
  if(!cur_up && prev_up){
    if (sel == -1){
      item += 1;
      item %= 5;
    }
    if(sel == 0){
    }
    if(sel == 1){
      frame_delay_s --;
    }
    if(sel == 2){
      down_time_s --;
    }
    if(sel == 3){
      frame_count --;
    }
  }
  prev_up = cur_up;

  int cur_down = digitalRead(9);
  if(!cur_down && prev_down){
    if (sel == -1){
      item -= 1;
      if (item < 0) item = 4;
    }
    if(sel == 0){
    }
    if(sel == 1){
      frame_delay_s ++;
    }
    if(sel == 2){
      down_time_s ++;
    }
    if(sel == 3){
      frame_count ++;
    }
  }
  prev_down = cur_down;
}
