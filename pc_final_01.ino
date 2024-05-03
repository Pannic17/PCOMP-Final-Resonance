#define USE_ARDUINO_INTERRUPTS true
#include <PulseSensorPlayground.h>
#include "AccelStepper.h"
#include "FastLED.h"

int PinPress0 = A0;
int PinPress1 = A5;
int PinHeart0 = A2;
int PinHeart1 = A3;

const int PulseI0 = 0;
const int PulseI1 = 1;

 
// 电机步进方式定义
#define FULLSTEP 4    //全步进参数
#define HALFSTEP 8    //半步进参数
 
// 定义步进电机引脚 
#define motor1Pin1  8     // 一号28BYJ48连接的ULN2003电机驱动板引脚 in1
#define motor1Pin2  9     // 一号28BYJ48连接的ULN2003电机驱动板引脚 in2
#define motor1Pin3  10    // 一号28BYJ48连接的ULN2003电机驱动板引脚 in3
#define motor1Pin4  11  

PulseSensorPlayground pulseSensor(2);
AccelStepper stepper1(HALFSTEP, motor1Pin1, motor1Pin3, motor1Pin2, motor1Pin4);

int lastE[2][10] = {
    {0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0}
};

CRGB Scolor[20] = {
  0xf56eb3, 0xcb1c8d, 0x7f167f, 0x6d67e4, 0x46C2cb, 0xffe15d,
  0xf49d1a, 0xdc3535, 0xcff5e7, 0x0d4c92, 0xd6e4e5, 0xeb6440,
  0xf0ff42, 0x379237, 0xff97c1, 0xe0144c, 0x4649ff, 0x1d1ce5,
  0x00f5ff
};

#define NUM_LEDS 12             // LED灯珠数量
#define DATA_PIN 5              // Arduino输出控制信号引脚
#define LED_TYPE WS2812B         // LED灯带型号
#define COLOR_ORDER RGB     // RGB灯珠中红色、绿色、蓝色LED的排列顺序
int colorI=0; 
uint8_t max_bright = 128;       // LED亮度控制变量，可使用数值为 0 ～ 255， 数值越大则光带亮度越高
CRGB leds[NUM_LEDS];

void setup() {
    int Threshold = 550;

    stepper1.setMaxSpeed(2000.0);    // 1号电机最大速度500 
    stepper1.setAcceleration(50.0);

    // Set input 0
    pulseSensor.analogInput(PinHeart0, PulseI0);
    pulseSensor.setThreshold(Threshold, PulseI0);
    // Set input 1
    pulseSensor.analogInput(PinHeart1, PulseI1);
    pulseSensor.setThreshold(Threshold, PulseI1);

    if (pulseSensor.begin()) {
        Serial.println("We create a pulseSensor Object!");
    }

    LEDS.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);  // 初始化灯带
    FastLED.setBrightness(max_bright);   

}


int time = -30000;
int target = 200;
void loop() {

  if ( stepper1.currentPosition() == 0){
    stepper1.moveTo(target);
    for( int i = 0; i < NUM_LEDS; i++) {
      // Serial.println(Scolor[6]);
      leds[i] = Scolor[0];
    }
    FastLED.show();
  }
  //   // 1号电机转动半周
  //   stepper1.moveTo(2048);  
  // trigger();

  // stepper1.run();
  if (stepper1.currentPosition() < target) {
    stepper1.run();
  }
  
  // put your main code here, to run repeatedly:

  if (time >= 30000) {
    time = -30000;
    int step = listen();
    // for( int i = 0; i < NUM_LEDS; i++) {
    //     // Serial.println(Scolor[6]);
    //   leds[i] = Scolor[colorI];
    // }
    // colorI += 1;
    if (colorI > 20) colorI = 0;
    FastLED.show();
    if (step > 0) {
      target += step;
      stepper1.moveTo(target);
      Serial.println(target);
    }
  }
  time += 1;

    // fuck you that's event
    
}

int listen() {
    int event0 = 0;
    int event1 = 0;
    // fuck arduino see I recovered heart;
    bool heart0 = readHeart(PulseI0);
    if (heart0) event0 += 1;
    bool heart1 = readHeart(PulseI1);
    if (heart1) event1 += 1;

    // fuck arduino this is analog pressure;
    bool touch0 = readTouch(PinPress0);
    if (touch0) event0 += 2;
    bool touch1 = readTouch(PinPress1);
    if (touch1) event1 += 2;

    int flag1;
    int flag2;

    pop10push(lastE[0], event0);
    pop10push(lastE[1], event1);

    int step = 0;

    if (event0 != 0 && event1 != 0) {
        // TODO: trigger special effect
        step += 100;
        for( int i = 0; i < NUM_LEDS; i++) {
        // Serial.println(Scolor[6]);
          leds[i] = Scolor[colorI];
        }
        colorI += 1;
        Serial.println("匹配成功");
    } else {
      if (event0 != 0) {
        step += check_last(event0, 1, 0);
      } else if (event1 != 0) {
        step += check_last(event1, 0, 1);
      }
    }
    return step;
}

int check_last(int event, int check, int user) {
    // lastUe: the user who trigger the event
    // lastCe: the list of operate used for check
    int count = 0;
    int multi = 1;
    bool touch = 0;
    for (int i = 9; i >= 0; i--) {
        count += lastE[check][i];
        if (lastE[check][i] != 0) {
            multi *= lastE[check][i];
        }
    }
    
    if (count == 0) {
        // 可恶，对方没有心动！
        // Serial.println("可恶，对方没有心动！");
        Serial.println("对方使用了龟壳功");
        return 1;
    } else {
        Serial.print(multi);
        Serial.print(":");
        Serial.print(event);
        Serial.print("##");
        Serial.println(user);
        // Serial.println("让我看看是谁心乱了？");
        if ((event * multi) % 2 == 0) {
          if (event == 2) {
            // touch meet touch
            Serial.println("葵花点穴手");
            return 50;
          } else {
            // touch meet heart
            Serial.println("心里有事了");
            return 20;
          }
        } else {
          if (event == 2) {
            // touch meet heart
            Serial.println("葵花解穴手");
            return 20;
          } else {
            // heart meet heart
            Serial.println("漏一拍心跳");
            return 10;
          }
        }
    }
}


void pop10push(int* last, int number) {
    for (int i = 9; i >= 0; i--) {
//        Serial.print(last[i]);
//        Serial.print("/");
        if (i == 0) {
            last[i] = number;
        } else {
            last[i] = last[i - 1];
        }
    }
}

void resetLast(int* last) {
    for (int i = 0; i < 10; i++) {
        last[i] = 0;
    }
}

bool readHeart(int pin){
    int myBPM = pulseSensor.getBeatsPerMinute(pin);
    if(pulseSensor.sawStartOfBeat(pin)){
        Serial.print("♥HeartBeat  ###  ");
        Serial.print("INPUT: ");
        Serial.print(pin);
        Serial.print("  |||  ");
        Serial.print("BPM: ");
        Serial.println(myBPM); //Heartbeat
        return 1;
    }
    else{
        return 0;
    }
}

bool readTouch(int pin) {
    int fsrReading = analogRead(pin);
    if(fsrReading > 300){
        Serial.print("TouchTouch  ###  ");
        Serial.print("INPUT: ");
        Serial.print(pin);
        Serial.print("  |||  ");
        Serial.print("PSR: ");
        Serial.println(fsrReading);
        return 1;
    }
    else{
        return 0;
    }
}

