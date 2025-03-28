#include <Arduino.h>
#include <TaskScheduler.h>
#include <PinChangeInterrupt.h>

// 기본 신호등 기능에서 각 LED의 점등시간
const int RED_Light = 2000;                  
const int YELLOW_Light = 500;                
const int GREEN_Light  = 2000;               
const int GREEN_BLINK = 1000;                
const int GREEN_BLINK_INTERVAL = 166;        

// 각 버튼과 연결할 핀 설정
const int BUTTON_1 = 4; 
const int BUTTON_2 = 5;
const int BUTTON_3 = 6;
const int POTENTIOMETER_PIN = A0;  

unsigned long lastBrightnessUpdate = 0; // 마지막으로 밝기 값을 전송한 시간
const unsigned long brightnessInterval = 500; // 밝기 값 전송 간격(ms)


// 시리얼 입력 (r,b,0,1)을 받을 버퍼
char serialCommand;

// 인터럽트 함수 선언
void button1Interrupt();
void button2Interrupt();
void button3Interrupt();

void stopAllTask();    // 모든 기능 중지
void Restart_Traffic();     // 신호등 기능 재시작
void adjustBrightness();    // 가변저항을 통한 밝기 조절 함수 
void processSerialInput();  // 시리얼 입력 처리 함수 
void stopState();             // 모든 기능 중지 함수

bool redOE(); 
void redOD();
bool yellow1OE(); 
void yellow1OD();
bool greenOE(); 
void greenOD();
bool greenBlinkOE(); 
void greenBlinkCB(); 
void greenBlinkOD();
bool yellow2OE(); 
void yellow2OD();
void allLEDBlinkCB(); 
void allLEDBlinkOD();

// TaskScheduler 객체 생성
Scheduler ts;

// Task 객체 생성
Task red(RED_Light, TASK_ONCE, NULL, &ts, false, redOE, redOD); 
Task yellow1(YELLOW_Light, TASK_ONCE, NULL, &ts, false, yellow1OE, yellow1OD);
Task green(GREEN_Light, TASK_ONCE, NULL, &ts, false, greenOE, greenOD);
Task greenBlink(GREEN_BLINK_INTERVAL, GREEN_BLINK / GREEN_BLINK_INTERVAL, greenBlinkCB, &ts, false, greenBlinkOE, greenBlinkOD);
Task yellow2(YELLOW_Light, TASK_ONCE, NULL, &ts, false, yellow2OE, yellow2OD);
// 모든 LED 깜빡임 Task
Task B2_allblink(500, TASK_FOREVER, allLEDBlinkCB, &ts, false, NULL, allLEDBlinkOD);

// 각 버튼의 상태를 저장할 변수
bool Traffic_State = true; // 신호등 기능 상태
bool B1_State = false;     // 버튼 1 상태
bool B2_State = false;     // 버튼 2 상태
bool B3_State = true;     // 버튼 3 상태
// 여기서 각각의 상태를 false로 맞추고 시작함
// 버튼을 누르면 true가 되고 true일 때 다시 누르면 false가 됨
// 여기서 B3_State랑 Traffic_State가 달라도 괜찮은 건가?


// 아두이노가 실행될 때 처음 한 번만 실행되는 함수
void setup() {
  Serial.begin(9600); // 시리얼 통신 시작
  Serial.println("Arduino assign start");

  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(BUTTON_1, INPUT_PULLUP);
  pinMode(BUTTON_2, INPUT_PULLUP);
  pinMode(BUTTON_3, INPUT_PULLUP);

  // 버튼 인터럽트 설정
  attachPCINT(digitalPinToPCINT(BUTTON_1), button1Interrupt, FALLING);
  attachPCINT(digitalPinToPCINT(BUTTON_2), button2Interrupt, FALLING);
  attachPCINT(digitalPinToPCINT(BUTTON_3), button3Interrupt, FALLING);

  // 신호등 기능 시작 (red부터)
  red.restartDelayed();
}

// task들을 주기적으로 실행
void loop() {
  ts.execute(); // TaskScheduler 실행
  adjustBrightness(); // 밝기 조절
  processSerialInput(); // 시리얼 입력 처리 (r, b, 0, 1)

  // 가변저항 값을 읽고 p5.js로 전송
  if (millis() - lastBrightnessUpdate >= brightnessInterval) {
    lastBrightnessUpdate = millis(); // 마지막 전송 시간 갱신

    // 가변저항 값 읽기
    int Pin_Value = analogRead(POTENTIOMETER_PIN); // 0~1023 값 읽기
    int Brightness = map(Pin_Value, 0, 1023, 0, 255); // 0~255로 변환

    // p5.js로 밝기 값 전송
    Serial.print("BRIGHTNESS: ");
    Serial.println(Brightness);
  }
}
/*
r: Red led only On 
b: All led Blink On
0: traffic light off
1: traffic light on
*/


// State를 모두 false로 변경
void stopState() {
  // 상태 플래그 전부 OFF
  B1_State = false; 
  B2_State = false;
  B3_State = false;
  Traffic_State = false;

  // stopAllTask();
}
// Task를 중지시킴(LED off)
void stopAllTask() {
  red.abort(); 
  yellow1.abort(); 
  green.abort(); 
  greenBlink.abort(); 
  yellow2.abort(); 
  B2_allblink.abort();

  // 모든 LED 끄기
  digitalWrite(9, LOW); 
  digitalWrite(10, LOW); 
  digitalWrite(11, LOW);
  Serial.println("All LED OFF");
}


// 신호등 기능 재시작 함수(Traffic_State를 true로 변경)
void Restart_Traffic() {
  Traffic_State = true; // 신호등 기능 시작
  red.restartDelayed(); // 빨간 LED 부터
  Serial.println("Traffic restart");
}


// 시리얼 포트로 명령을 읽어와서 아두이노 동작을 수행하는 함수
void processSerialInput() {
  if (Serial.available() > 0) { // 시리얼 입력이 있는 경우
    serialCommand = Serial.read(); // 시리얼 입력을 읽음
    String input = Serial.readStringUntil('\n'); // 한 줄 읽기
    

    // 밝기 조절
    if (input.startsWith("BRIGHTNESS_SET:")) {
      int newBrightness = input.substring(15).toInt(); // 값 추출
      analogWrite(9, digitalRead(9) * newBrightness);
      analogWrite(10, digitalRead(10) * newBrightness);
      analogWrite(11, digitalRead(11) * newBrightness);
      Serial.print("Arduino brightness set: "); 
      Serial.println(newBrightness);
    }

    // 시리얼 입력에 따른 동작 설정
    switch (serialCommand) {

      case '1': // 신호등 ON
        Serial.println("Arduino Traffic On");
        stopState(); // State를 모두 false로 설정
        stopAllTask(); // Task 종료

        B3_State = true; // 신호등 ON
        Restart_Traffic(); // 신호등 기능 재시작
        break;

      case '0': // 신호등 OFF
        Serial.println("Arduino Traffic Off");
        stopState(); // State를 모두 false로 설정
        stopAllTask(); // Task 종료
        break;

      case 'r': // 빨간불 켜기
        Serial.println("Arduino RED LED On");
        stopState();
        stopAllTask();

        B1_State = true; // 빨간 LED only True 설정
        digitalWrite(9, HIGH); // 빨간불 켜기 (아두이노의 핀 설정)
        break;

      case 'b': // 모든 LED 깜빡이기
        Serial.println("Arduino All LED Blink On");
        stopState();
        stopAllTask();

        B2_State = true; // All led blink on
        B2_allblink.restartDelayed(); // 깜빡임 Task 시작
        break;
        
      default:
        Serial.println("fail");
        break;
    }
  }
}



// 가변저항을 통한 밝기 조절
void adjustBrightness() {
  int Pin_Value = analogRead(POTENTIOMETER_PIN); // 가변저항 값 읽기 (0~1023)
  int Brightness = map(Pin_Value, 0, 1023, 0, 255); // 밝기 값으로 변환 (0~255)

  // 현재 LED 핀(9,10,11)의 디지털 상태를 곱해서 PWM 출력
  analogWrite(9, digitalRead(9) * Brightness);
  analogWrite(10, digitalRead(10) * Brightness);
  analogWrite(11, digitalRead(11) * Brightness);
}


//----------------------------------//
//      버튼 인터럽트 함수 정의       //
//----------------------------------//



//------------버튼 1 인터럽트 함수-----------
void button1Interrupt() { 
  // 버튼 1이 눌린 경우 실행

  // 버튼1이 꺼져있었다면 (켜야지 true로 변경)
  if (B1_State == false) {
    stopState(); stopAllTask();
    Serial.println("BUTTON1_ON");
    B1_State = true;  // true로 변경해줌 (B1만 true인 상태)
    digitalWrite(9, HIGH); // 아두이노 빨간불 켜기
  } // B1만 true



  // 버튼1이 켜져있었다면
  else if (B1_State == true) {

    B1_State = !B1_State; // false로 변경

    // 버튼 1이 눌린 상태에서 버튼 2가 눌린다면 
    if (B2_State == true) {
      Serial.println("Red Led only OFF / All Blink ON");
      stopState(); stopAllTask(); 
      B2_State = true;
      B2_allblink.restartDelayed(); // 깜빡이기 재시작
    } // B2만 true인 상태

    // 버튼 1이 눌린 상태에서 버튼 3이 눌린다면
    else if (Traffic_State == true) {
      Serial.println("Red Led only OFF / Traffic ON");
      stopState(); stopAllTask();
      Restart_Traffic(); // 신호등 기능 재시작
    }

    // 버튼1이 다시 눌린 경우 (T->F)
    else if (B1_State == false){
      Serial.println("BUTTON1_OFF");
      stopAllTask(); stopState(); // 모든 기능 중지
      Restart_Traffic(); // Traffic은 T가 된 상태
    } // Traffic만 true가 된 상태임
  }
}


//------------버튼 2 인터럽트 함수-----------
void button2Interrupt() {

  // B2_State = !B2_State;

  // 버튼 2가 꺼져있는 상태였다면(깜빡이게 만들어야 함)
  if (B2_State == false) {
    Serial.println("BUTTON2_ON");
    stopAllTask(); stopState();
    B2_State = true;  // true로 변경
    B2_allblink.restartDelayed(); // 깜빡이기 시작
  } // B2만 true인 상태(깜빡이는 중~)
  
  // 버튼 2가 실행되던 상태였다면 (끄고 다른기능 해야지)
  else if (B2_State == true) { 

    B2_State = !B2_State; // 상태 반전 T->F
    
    // 버튼 2가 눌린 상태에서 버튼 1이 눌린다면
    // 여기서 눌렀을 때 true가 되는 게 맞나?
    if (B1_State == true) { 
      Serial.println("All LED Blink OFF / Red LED only ON");
      stopState(); stopAllTask(); // 모든 기능 중지

      B1_State = true;  // 빨간 LED only ON
      digitalWrite(9, HIGH); // 빨간불 켜기
    }

    // 버튼 2가 눌린 상태에서 버튼 3이 눌린다면
    // 여기서 눌렀을 때 true가 되는 게 맞나?
    else if (Traffic_State == true) {
      Serial.println("All LED Blink OFF / Traffic ON");
      stopState(); stopAllTask();
      Restart_Traffic(); // 신호등 기능 재시작
    }

    // 버튼 2가 눌린 상태에서 다시 버튼 2가 눌린다면
    else if (B2_State == false) {
      Serial.println("BUTTON2_OFF");
      stopState(); stopAllTask();
      Restart_Traffic(); // 신호등 기능 재시작
    } // Traffic만 true가 됨
  }
}


//------------버튼 3 인터럽트 함수-----------
void button3Interrupt() { // 버튼을 눌렀을 때 실행됨

  // 버튼3이 꺼져있는 상태였으면 
  if (Traffic_State == false) {
    Serial.println("BUTTON3_ON");
    stopState(); stopAllTask();
    Restart_Traffic(); // 빨간 LED 부터
  } // Traffic만 true

  // 버튼3이 켜져있는 상태였다면(신호등 기능 실행 중)
  else if (Traffic_State == true) {
    Serial.println("BUTTON3_OFF");
    stopState(); stopAllTask();
  } // 전부 꺼버림 all false
}



// ----------------------------------------
// 신호등 로직(Task) 정의
// ----------------------------------------


bool redOE() { 
  if (!Traffic_State) return false; // 신호등 기능이 중지된 경우 false 반환
  digitalWrite(9, HIGH); // 빨간불 켜기
  Serial.println("RED");
  return true; 
}

void redOD() { 
  if (!Traffic_State) return; // 신호등 기능이 중지된 경우 함수 종료
  digitalWrite(9, LOW); // 빨간불 끄기
  yellow1.restartDelayed(); // 노란불 타이머 재시작
}

bool yellow1OE() { 
  if (!Traffic_State) return false; // 신호등 기능이 중지된 경우 false 반환
  digitalWrite(10, HIGH); // 노란불 켜기
  Serial.println("YELLOW"); 
  return true; 
}

void yellow1OD() { 
  if (!Traffic_State) return; // 신호등 기능이 중지된 경우 함수 종료
  digitalWrite(10, LOW); // 노란불 끄기
  green.restartDelayed(); // 초록불 타이머 재시작
}

bool greenOE() { 
  if (!Traffic_State) return false; // 신호등 기능이 중지된 경우 false 반환
  digitalWrite(11, HIGH); // 초록불 켜기
  Serial.println("GREEN");
  return true; 
}

void greenOD() { 
  if (!Traffic_State) return; // 신호등 기능이 중지된 경우 함수 종료
  digitalWrite(11, LOW); // 초록불 끄기
  greenBlink.restartDelayed(); // 초록불 깜빡임 타이머 재시작
}

bool greenBlinkOE() { 
  if (!Traffic_State) return false; // 신호등 기능이 중지된 경우 false 반환
  digitalWrite(11, LOW); // 초록불 끄기
  Serial.println("GREENBLINK");
  return true; 
}

void greenBlinkCB() { 
  if (!Traffic_State) return; // 신호등 기능이 중지된 경우 함수 종료
  digitalWrite(11, !digitalRead(11)); // 초록불 상태 반전
}

void greenBlinkOD() { 
  if (!Traffic_State) return; // 신호등 기능이 중지된 경우 함수 종료
  digitalWrite(11, LOW); // 초록불 끄기
  Serial.println("YELLOW2");
  yellow2.restartDelayed(); // 노란불 타이머 재시작
}

bool yellow2OE() { 
  if (!Traffic_State) return false; // 신호등 기능이 중지된 경우 false 반환
  digitalWrite(10, HIGH); // 노란불 켜기
  Serial.println("YELLOW2");
  return true; 
}

void yellow2OD() { 
  if (!Traffic_State) return; // 신호등 기능이 중지된 경우 함수 종료
  digitalWrite(10, LOW); // 노란불 끄기
  Serial.println("RED");
  red.restartDelayed(); // 빨간불 타이머 재시작
}

void allLEDBlinkCB() {
  bool state = digitalRead(9);  // 9번 핀의 LED 상태를 읽음
  state = !state;  // 상태 반전
  
  digitalWrite(9, state);  // 9번 핀의 LED 상태를 반전된 상태로 설정
  digitalWrite(10, state);  // 10번 핀의 LED 상태를 반전된 상태로 설정
  digitalWrite(11, state);  // 11번 핀의 LED 상태를 반전된 상태로 설정
}

void allLEDBlinkOD() {
  digitalWrite(9, LOW); // 9번 핀의 LED 끄기
  digitalWrite(10, LOW); // 10번 핀의 LED 끄기
  digitalWrite(11, LOW); // 11번 핀의 LED 끄기
}