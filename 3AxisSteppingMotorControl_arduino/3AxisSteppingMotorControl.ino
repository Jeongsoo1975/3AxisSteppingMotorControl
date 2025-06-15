// 3-Axis Stepper Motor Controller - AccelStepper Final Version
// 최종 핀 배열 및 가역 Homing 기능 적용 (디버깅 코드 추가)

#include <AccelStepper.h>

// --- 설정 섹션 ---

// 1. << 테스트 기능 >> 초기화(Homing) 루틴 자동 실행 여부 설정
const bool AUTO_HOME_ON_STARTUP = false; // true: 자동 원점 복귀, false: 수동 대기

// 2. 모터 드라이버 핀 연결 (사용자 지정)
#define X_DIR_PIN  2
#define X_STEP_PIN 3

#define Y_DIR_PIN  4
#define Y_STEP_PIN 5

#define Z_DIR_PIN  6
#define Z_STEP_PIN 7

// 3. 리미트 스위치 핀 연결 (사용자 지정 최종)
#define X_LIMIT_UPPER_PIN 8
#define X_LIMIT_LOWER_PIN 9

#define Y_LIMIT_UPPER_PIN 12
#define Y_LIMIT_LOWER_PIN 18 // A4

#define Z_LIMIT_UPPER_PIN 11
#define Z_LIMIT_LOWER_PIN 10

// 4. AccelStepper 객체 생성
AccelStepper xStepper(AccelStepper::DRIVER, X_STEP_PIN, X_DIR_PIN);
AccelStepper yStepper(AccelStepper::DRIVER, Y_STEP_PIN, Y_DIR_PIN);
AccelStepper zStepper(AccelStepper::DRIVER, Z_STEP_PIN, Z_DIR_PIN);

// --- 전역 변수 ---
String serialBuffer = "";
bool commandReady = false;
bool isMoving = false;

// --- setup() 함수 ---
void setup() {
  Serial.begin(9600); 
  Serial.println("AccelStepper CNC Controller Initialized.");
  Serial.println("Final pin map applied.");

  // 리미트 스위치 핀 입력으로 설정
  pinMode(X_LIMIT_LOWER_PIN, INPUT_PULLUP);
  pinMode(Y_LIMIT_LOWER_PIN, INPUT_PULLUP);
  pinMode(Z_LIMIT_LOWER_PIN, INPUT_PULLUP);
  pinMode(X_LIMIT_UPPER_PIN, INPUT_PULLUP);
  pinMode(Y_LIMIT_UPPER_PIN, INPUT_PULLUP);
  pinMode(Z_LIMIT_UPPER_PIN, INPUT_PULLUP);

  // 모터 기본 파라미터 설정 (사용자의 환경에 맞게 튜닝하세요)
  xStepper.setMaxSpeed(1000);
  yStepper.setMaxSpeed(1000);
  zStepper.setMaxSpeed(500);

  xStepper.setAcceleration(500);
  yStepper.setAcceleration(500);
  zStepper.setAcceleration(250);

  if (AUTO_HOME_ON_STARTUP) {
    performHoming(false); // 기본 Homing (반전되지 않음)
  } else {
    Serial.println("Auto-homing is DISABLED. Ready for manual commands.");
  }
}

// --- loop() 함수 ---
void loop() {
  // 시리얼 데이터 수신 처리
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '>') { commandReady = true; } 
    else { serialBuffer += c; }
  }

  // 완전한 명령어가 수신되면 처리합니다.
  if (commandReady) {
    processCommand();
    serialBuffer = "";
    commandReady = false;
  }
  
  // AccelStepper 라이브러리의 핵심. 각 모터의 run() 함수를 계속 호출해줘야 합니다.
  xStepper.run();
  yStepper.run();
  zStepper.run();

  // 모든 이동이 완료되었는지 확인하고 메시지를 출력합니다.
  if (isMoving && xStepper.distanceToGo() == 0 && yStepper.distanceToGo() == 0 && zStepper.distanceToGo() == 0) {
    isMoving = false;
    Serial.println("Movement complete.");
    printCurrentPosition();
  }
}

// --- 명령어 처리 함수 ---
void processCommand() {
  if (serialBuffer.startsWith("<")) serialBuffer.remove(0, 1);
  Serial.print("Received command: "); Serial.println(serialBuffer);

  if (serialBuffer.equalsIgnoreCase("HOME")) {
    isMoving = true;
    performHoming(false); // 기본 Homing 호출
  } 
  else if (serialBuffer.equalsIgnoreCase("HOME_REVERSED")) {
    isMoving = true;
    performHoming(true); // 반전 Homing 호출
  }
  else {
    isMoving = true;
    long targetX = xStepper.currentPosition();
    long targetY = yStepper.currentPosition();
    long targetZ = zStepper.currentPosition();

    if (serialBuffer.indexOf('X') != -1) targetX = parseValue(serialBuffer, 'X');
    if (serialBuffer.indexOf('Y') != -1) targetY = parseValue(serialBuffer, 'Y');
    if (serialBuffer.indexOf('Z') != -1) targetZ = parseValue(serialBuffer, 'Z');

    xStepper.moveTo(targetX);
    yStepper.moveTo(targetY);
    zStepper.moveTo(targetZ);
  }
}

// --- Homing 함수 ---
void performHoming(bool reversed) {
  if (reversed) {
    Serial.println("Reversed Homing sequence started (X -> Y -> Z)...");
  } else {
    Serial.println("Homing sequence started (X -> Y -> Z)...");
  }
  isMoving = false; 

  // 방향 및 속도, 사용할 리미트 스위치 핀 결정을 위한 변수
  int homing_direction = reversed ? 1 : -1;
  int limit_switch_pin_x = reversed ? X_LIMIT_UPPER_PIN : X_LIMIT_LOWER_PIN;
  int limit_switch_pin_y = reversed ? Y_LIMIT_UPPER_PIN : Y_LIMIT_LOWER_PIN;
  int limit_switch_pin_z = reversed ? Z_LIMIT_UPPER_PIN : Z_LIMIT_LOWER_PIN;

  // 1. X축 Homing
  Serial.println("Homing X-Axis...");
  xStepper.setSpeed(800 * homing_direction);
  xStepper.moveTo(1000000 * homing_direction);
  // --- [디버깅 추가] ---
  while (digitalRead(limit_switch_pin_x) == HIGH) {
    xStepper.runSpeed();
    // 100스텝마다 한 번씩 현재 핀 상태를 출력하여 시리얼 모니터 과부하 방지
    if (abs(xStepper.currentPosition()) % 100 == 0) {
      Serial.print("Homing X... Reading Pin ");
      Serial.print(limit_switch_pin_x);
      Serial.print(": ");
      Serial.println(digitalRead(limit_switch_pin_x));
    }
  }
  xStepper.stop();
  xStepper.setCurrentPosition(0);
  Serial.println("X-Limit hit. Backing off...");
  xStepper.moveTo(50 * -homing_direction); // 반대 방향으로 백오프
  while(xStepper.distanceToGo() != 0) { xStepper.run(); }
  xStepper.setCurrentPosition(50 * -homing_direction);
  Serial.println("X-Axis homing complete.");

  // 2. Y축 Homing
  Serial.println("Homing Y-Axis...");
  yStepper.setSpeed(800 * homing_direction);
  yStepper.moveTo(1000000 * homing_direction);
  while (digitalRead(limit_switch_pin_y) == HIGH) { yStepper.runSpeed(); }
  yStepper.stop();
  yStepper.setCurrentPosition(0);
  Serial.println("Y-Limit hit. Backing off...");
  yStepper.moveTo(50 * -homing_direction);
  while(yStepper.distanceToGo() != 0) { yStepper.run(); }
  yStepper.setCurrentPosition(50 * -homing_direction);
  Serial.println("Y-Axis homing complete.");

  // 3. Z축 Homing
  Serial.println("Homing Z-Axis...");
  zStepper.setSpeed(500 * homing_direction);
  zStepper.moveTo(1000000 * homing_direction);
  while (digitalRead(limit_switch_pin_z) == HIGH) { zStepper.runSpeed(); }
  zStepper.stop();
  zStepper.setCurrentPosition(0);
  Serial.println("Z-Limit hit. Backing off...");
  zStepper.moveTo(50 * -homing_direction);
  while(zStepper.distanceToGo() != 0) { zStepper.run(); }
  zStepper.setCurrentPosition(50 * -homing_direction);
  Serial.println("Z-Axis homing complete.");
  
  Serial.println("Homing sequence finished. System ready.");
  printCurrentPosition();
}

// --- 유틸리티 함수 ---
long parseValue(String &data, char key) {
  int keyIndex = data.indexOf(key);
  // 키가 없으면 현재 위치를 반환하도록 수정하여, 단일 축 이동 시 다른 축이 움직이지 않도록 함
  if (keyIndex == -1) {
    if (key == 'X') return xStepper.currentPosition();
    if (key == 'Y') return yStepper.currentPosition();
    if (key == 'Z') return zStepper.currentPosition();
  }
  int commaIndex = data.indexOf(',', keyIndex);
  if (commaIndex == -1) commaIndex = data.length();
  return data.substring(keyIndex + 1, commaIndex).toInt();
}

void printCurrentPosition() {
    Serial.print("Current Position: X="); Serial.print(xStepper.currentPosition());
    Serial.print(" Y="); Serial.print(yStepper.currentPosition());
    Serial.print(" Z="); Serial.println(zStepper.currentPosition());
}
