// Arduino_Light_Tracker
#include <Servo.h> 
#include <SoftwareSerial.h>

#define light_sensor_0 A0
#define light_sensor_1 A1
#define light_sensor_2 A2
#define light_sensor_3 A3
#define servo_UD_pin 9
#define servo_LR_pin 11
#define software_RXD 3
#define software_TXD 2

#define scan_delay_time 50 // 빛 탐지 지연시간
#define control_delay_time 5 // 서보 제어 지연시간
#define error_range 50  // 빛 탐지 오차
#define range 100  // 가속을 위한 나누기 기준점  [아직 사용 안함]
#define servo_speed 3 // 서보모터 회전 속도
#define serial_delay 200  // 방위 정보 통신 딜레이
#define light_low_value 500 // 빛의 세기 기준점 (이 값의 아래면 밤)
#define light_low_count_value 30 // 밤, 낮 판명 기준 카운트
#define servoUD_lineup 83 // 정렬 초기값
#define servoLR_lineup 97 // 정렬 초기값

int light_sensor_value[4] = { 0 };  // 조도센서 0 ~ 3 번 값
int UD_servo_value = servoUD_lineup; // 상 하 조정 서보 값
int LR_servo_value = servoLR_lineup; // 좌 우조정 서보 값
int UD_compare_value = 0; // 상 하 값의 차 (양수면 광원은 위, 음수면 광원은 아래)
int LR_compare_value = 0; // 좌 우 값의 차
int UD_sum_value = 0; // 상 하 값의 합 (일정 값 아래면 어두움)
int LR_sum_value = 0; // 좌 우 값의 합 (일정 값 아래면 어두움)
int mode_ = 1; // 모드 변환, 0 = 추적 모드, 1 = 수리 모드, 2 = 테스트 모드, 3 = 정렬 모드
int mode_change = mode_; // 모드 변환 전 저장 (현 모드와 이전 모드가 다를 경우 모드변환 확인)
int light_low_count = 0;  // 밤, 낮 확인 (30 이상 : 밤, -30 이하 : 낮)

Servo servo_UD;
Servo servo_LR;

SoftwareSerial BTSerial(software_TXD, software_RXD);  // (TXD, RXD)

void setup()
{
  Serial.begin(9600);
  BTSerial.begin(9600);
  pinMode(light_sensor_0, INPUT);
  pinMode(light_sensor_1, INPUT);
  pinMode(light_sensor_2, INPUT);
  pinMode(light_sensor_3, INPUT);

  servo_UD.attach(servo_UD_pin);
  servo_LR.attach(servo_LR_pin);
  
  BTSerial.println(mode_);
  Serial.println(" 빛 추적기 가동중.. ");
  Serial.println(" [수리 모드]");
}

void loop()
{
  serial_communicative(); // 시리얼 통신
  mode_action();  // 지정 모드 실행 및 모드변환 여부 확인
}

void mode_action()  // 모드 변환
{
  switch(mode_)
  {
    case 0 :  // 추적 모드
      value_update();
      break;

    case 1 :  // 수리 모드
      light_tracker_repair();
      break;

    case 2 :  // 테스트 모드
      light_tracker_Test();
      break;
      
    case 3 :  // 정렬 모드
      light_tracker_line_up();
      break;
  }
  
  if(mode_ != mode_change)
  {
    BTSerial.println(mode_);
    mode_change = mode_;
  }
    
}

void sensor_value_print() // 센서 값 시리얼 모니터에 출력
{
  Serial.print(" Sensor0 = ");  Serial.print(light_sensor_value[0]);
  Serial.print(" Sensor1 = ");  Serial.print(light_sensor_value[1]);
  Serial.print(" Sensor2 = ");  Serial.print(light_sensor_value[2]);
  Serial.print(" Sensor3 = ");  Serial.print(light_sensor_value[3]);
  Serial.print(" UD_Servo = ");  Serial.print(UD_servo_value);
  Serial.print(" LR_Servo = ");  Serial.print(LR_servo_value);
  Serial.print("\n");
}

void sensor_scan()  //  센서 탐지
{
  light_sensor_value[0] = analogRead(light_sensor_0);
  light_sensor_value[1] = analogRead(light_sensor_1);
  light_sensor_value[2] = analogRead(light_sensor_2);
  light_sensor_value[3] = analogRead(light_sensor_3);
  
  UD_compare_value = light_sensor_value[0] - light_sensor_value[1];
  UD_sum_value = light_sensor_value[0] + light_sensor_value[1];
  
  LR_compare_value = light_sensor_value[2] - light_sensor_value[3];
  LR_sum_value = light_sensor_value[2] + light_sensor_value[3];

  sensor_value_print();
  
  delay(scan_delay_time);
}

void servo_control(int UD_value, int LR_value)  // 서보모터 제어
{
  servo_UD.write(UD_value);
  servo_LR.write(LR_value);
}

void value_update() // 추적 모드(값 업데이트)
{
  sensor_scan();

  if(UD_sum_value > light_low_value)  // UD서보모터 제어
  {
    light_low_count = 0;  // 빛 감지시 카운트 초기화
    if (UD_compare_value > error_range) UD_servo_value += servo_speed; // UD > error_range 이면 광원은 0 쪽에 있다
    if (UD_compare_value < -error_range) UD_servo_value -= servo_speed; // UD < -error_range 이면 광원은 1 쪽에 있다
  }
  
  if(LR_sum_value > light_low_value)  // LR서보모터 제어
  {
    light_low_count = 0;  // 빛 감지시 카운트 초기화
    if(UD_servo_value >= servoUD_lineup)  // 
    {
      if (LR_compare_value > error_range) LR_servo_value -= servo_speed; // LR > error_range 이면 광원은 2 쪽에 있다
      if (LR_compare_value < -error_range) LR_servo_value += servo_speed; // LR < -error_range 이면 광원은 3 쪽에 있다
    }
    else if(UD_servo_value < servoUD_lineup)
    {
      if (LR_compare_value > error_range) LR_servo_value += servo_speed; // UD > error_range 이면 광원은 2 쪽에 있다
      if (LR_compare_value < -error_range) LR_servo_value -= servo_speed; // UD < -error_range 이면 광원은 3 쪽에 있다
    }
  }
  
  if(UD_sum_value < light_low_value && LR_sum_value < light_low_value)  // 모두 어두운게 지속될 시 카운트 다운 시작
  {
    Serial.println(" 빛이 부족합니다. ");
    light_low_count++;
    if(light_low_count > light_low_count_value)  // 일정 값이 넘어가면 밤으로 간주하여 정렬모드로 돌입
    {
      Serial.println(" 밤이되어 정렬 모드로 변환합니다.");
      light_low_count = 0;
      mode_ = 3;
    }
  }
  
  
  if(UD_servo_value > 180)  // 서보모터 제어 범위 초과시 초기화
    UD_servo_value = 180;
  if(UD_servo_value < 0)
    UD_servo_value = 0;

  if(LR_servo_value > 180)  // "
    LR_servo_value = 180;
  if(LR_servo_value < 0)
    LR_servo_value = 0;
  
  servo_control(UD_servo_value, LR_servo_value);  // 서보모터 제어
}

void light_tracker_repair()  // 수리 모드
{
  servo_UD.write(servoUD_lineup);
  servo_LR.write(servoLR_lineup);
}

void light_tracker_line_up()  // 정렬 모드
{
  servo_UD.write(servoUD_lineup);
  servo_LR.write(servoLR_lineup);
  sensor_scan();
  
  if(UD_sum_value > light_low_value)
  {
    Serial.println(" 빛을 감지했습니다. ");
    light_low_count--;
    if(light_low_count < -light_low_count_value)
    {
      Serial.println(" 낮이되어 추적 모드로 변환합니다.");
      light_low_count = 0;
      mode_ = 0;
    }
  }
}

void light_tracker_Test() // 테스트 모드
{
  int Test_mode_direction = 0;  // 돌아가는 방향 0 => 반시계로, 1 => 시계로
  servo_UD.write(0);  // 추적기 정렬
  servo_LR.write(0);
  delay(100);
  while(mode_ == 2)
  {
    if (Test_mode_direction == 0 && mode_ == 2)
    {
      Serial.println(" 테스트 모드 가동중");
      while(UD_servo_value < 180 && LR_servo_value < 180 && mode_ == 2)
      {
        UD_servo_value += servo_speed;
        LR_servo_value += servo_speed;
        servo_control(UD_servo_value, LR_servo_value);
        serial_communicative();
        delay(100);
      }
       Test_mode_direction = 1; // 방향 전환
    }
    else if (Test_mode_direction)
    {
      while(UD_servo_value > 0 && LR_servo_value > 0 && mode_ == 2)
      {
        UD_servo_value -= servo_speed;
        LR_servo_value -= servo_speed;
        servo_control(UD_servo_value, LR_servo_value);
        serial_communicative();
        delay(100);
      }
      Test_mode_direction = 0;  // 방향 전환
    }
  }
}

void serial_communicative() // 시리얼 통신 및 모드 변환
{
char BTdata_str; // 소프트웨어 시리얼 데이터
char data_str; // 시리얼 데이터

  if(BTSerial.available())
  {
    BTdata_str = BTSerial.read();
    //Serial.println(BTdata_str);

    switch(BTdata_str)
    {
      case 't' :  // 연결 테스트
        Serial.println(" 연결 테스트 ");
        BTSerial.println(mode_);
        break;
      
      case 's' :  // LR 위치 추적, // UD 위치 추적
        delay(serial_delay);
        Serial.println(" LR 위치 추적 ");
        BTSerial.println(LR_servo_value);
        delay(serial_delay);
        Serial.println(" UD 위치 추적 ");
        BTSerial.println(UD_servo_value);
        delay(serial_delay);
        break;
        
      case '0' :  // 추적 모드
        Serial.println(" 추적 모드로 변환 ");
        mode_ = 0;
        break;

      case '1' :  // 수리 모드
        Serial.println(" 수리 모드로 변환 ");
        mode_ = 1;
        break;

      case '2' :  // 테스트 모드
        Serial.println(" 테스트 모드로 변환 ");
        mode_ = 2;
        break;

      case '3' :  // 정렬 모드
        Serial.println(" 정렬 모드로 변환 ");
        mode_ = 3;
        break;
    }
  }

  /*
  if(Serial.available())
  {
    data_str = Serial.read();
    BTSerial.println(data_str);
  }
  */
}

