// 3-Axis Stepper Motor Controller - AccelStepper Final Version
// 최종 핀 배열 및 가역 Homing 기능 적용 + 동적 속도 제어

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
int homingSpeed = 800;      // 기본 홈잉 속도
int backoffPulses = 50;     // 기본 백오프 펄스 수

// --- setup() 함수 ---
void setup() {
  Serial.begin(9600); 
  Serial.println("AccelStepper CNC Controller Initialized.");
  Serial.println("Direct pulse homing system with delay-based speed control.");

  // 스테퍼 모터 핀 출력으로 설정
  pinMode(X_STEP_PIN, OUTPUT);
  pinMode(X_DIR_PIN, OUTPUT);
  pinMode(Y_STEP_PIN, OUTPUT);
  pinMode(Y_DIR_PIN, OUTPUT);
  pinMode(Z_STEP_PIN, OUTPUT);
  pinMode(Z_DIR_PIN, OUTPUT);
  
  // 초기 상태 설정
  digitalWrite(X_STEP_PIN, LOW);
  digitalWrite(X_DIR_PIN, LOW);
  digitalWrite(Y_STEP_PIN, LOW);
  digitalWrite(Y_DIR_PIN, LOW);
  digitalWrite(Z_STEP_PIN, LOW);
  digitalWrite(Z_DIR_PIN, LOW);

  // 리미트 스위치 핀 입력으로 설정
  pinMode(X_LIMIT_LOWER_PIN, INPUT_PULLUP);
  pinMode(Y_LIMIT_LOWER_PIN, INPUT_PULLUP);
  pinMode(Z_LIMIT_LOWER_PIN, INPUT_PULLUP);
  pinMode(X_LIMIT_UPPER_PIN, INPUT_PULLUP);
  pinMode(Y_LIMIT_UPPER_PIN, INPUT_PULLUP);
  pinMode(Z_LIMIT_UPPER_PIN, INPUT_PULLUP);

  // 모터 기본 파라미터 설정 (높은 최대 속도로 설정하여 제한 해제)
  xStepper.setMaxSpeed(10000);  // 충분히 높은 최대 속도
  yStepper.setMaxSpeed(10000);  // 충분히 높은 최대 속도
  zStepper.setMaxSpeed(10000);  // 충분히 높은 최대 속도

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

  // 홈잉 명령어 처리 (속도와 백오프 파라미터 포함)
  if (serialBuffer.startsWith("HOME")) {
    isMoving = true;
    
    // 홈잉 속도 파싱 (S 파라미터)
    if (serialBuffer.indexOf(",S") != -1) {
      homingSpeed = parseValue(serialBuffer, 'S');
      Serial.print("Homing Speed set to: "); Serial.println(homingSpeed);
      
      // 동적으로 최대 속도 조정 (홈잉 속도보다 충분히 높게)
      int maxSpeedLimit = max(homingSpeed + 1000, 10000);
      xStepper.setMaxSpeed(maxSpeedLimit);
      yStepper.setMaxSpeed(maxSpeedLimit);
      zStepper.setMaxSpeed(maxSpeedLimit);
      Serial.print("Max speed adjusted to: "); Serial.println(maxSpeedLimit);
    }
    
    // 백오프 펄스 수 파싱 (B 파라미터)
    if (serialBuffer.indexOf(",B") != -1) {
      backoffPulses = parseValue(serialBuffer, 'B');
      Serial.print("Backoff Pulses set to: "); Serial.println(backoffPulses);
    }
    
    if (serialBuffer.startsWith("HOME_REVERSED")) {
      performHoming(true); // 반전 Homing 호출
    } else {
      performHoming(false); // 기본 Homing 호출
    }
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

// --- 직접 펄스 생성 홈잉 함수 ---
void performHoming(bool reversed) {
  if (reversed) {
    Serial.println("Reversed Homing sequence started (X -> Y -> Z)...");
  } else {
    Serial.println("Homing sequence started (X -> Y -> Z)...");
  }
  isMoving = false; 

  // 방향 및 사용할 리미트 스위치 핀 결정
  int homing_direction = reversed ? 1 : -1;
  int limit_switch_pin_x = reversed ? X_LIMIT_UPPER_PIN : X_LIMIT_LOWER_PIN;
  int limit_switch_pin_y = reversed ? Y_LIMIT_UPPER_PIN : Y_LIMIT_LOWER_PIN;
  int limit_switch_pin_z = reversed ? Z_LIMIT_UPPER_PIN : Z_LIMIT_LOWER_PIN;

  // 속도를 delay 시간으로 변환 (마이크로초)
  // homingSpeed는 steps/sec이므로, 1,000,000 / homingSpeed = 마이크로초 delay
  unsigned long stepDelay = 1000000UL / homingSpeed;
  
  Serial.print("Using homing speed: "); Serial.print(homingSpeed); 
  Serial.print(" steps/sec ("); Serial.print(stepDelay); Serial.println(" us delay)");
  Serial.print("Backoff pulses: "); Serial.println(backoffPulses);

  // 1. X축 Homing
  Serial.println("Homing X-Axis with direct pulse control...");
  homeAxisDirect(X_STEP_PIN, X_DIR_PIN, limit_switch_pin_x, homing_direction, stepDelay, "X");
  
  // 2. Y축 Homing  
  Serial.println("Homing Y-Axis with direct pulse control...");
  homeAxisDirect(Y_STEP_PIN, Y_DIR_PIN, limit_switch_pin_y, homing_direction, stepDelay, "Y");
  
  // 3. Z축 Homing
  Serial.println("Homing Z-Axis with direct pulse control...");
  homeAxisDirect(Z_STEP_PIN, Z_DIR_PIN, limit_switch_pin_z, homing_direction, stepDelay, "Z");
  
  // AccelStepper 위치 초기화 (다른 이동에서 계속 사용하므로)
  xStepper.setCurrentPosition(backoffPulses * -homing_direction);
  yStepper.setCurrentPosition(backoffPulses * -homing_direction);
  zStepper.setCurrentPosition(backoffPulses * -homing_direction);
  
  Serial.println("Homing sequence finished. System ready.");
  printCurrentPosition();
}

// --- 개별 축 직접 펄스 홈잉 함수 ---
void homeAxisDirect(int stepPin, int dirPin, int limitPin, int direction, unsigned long stepDelay, String axisName) {
  Serial.print("Starting "); Serial.print(axisName); Serial.println("-Axis homing...");
  Serial.print("Using step delay: "); Serial.print(stepDelay); Serial.println(" microseconds");
  
  // 현재 리밋 스위치 상태 확인
  bool limitState = digitalRead(limitPin);
  Serial.print("Initial limit switch state: "); 
  Serial.println(limitState == HIGH ? "HIGH (not triggered)" : "LOW (triggered)");
  
  // 만약 이미 리밋 스위치가 감지된 상태라면 먼저 해제
  if (limitState == LOW) {
    Serial.println("Limit switch already triggered! Moving away from limit first...");
    
    // 역방향으로 설정 (리밋에서 벗어나기 위해)
    digitalWrite(dirPin, direction > 0 ? LOW : HIGH);
    delay(10);
    
    // 리밋 스위치가 해제될 때까지 역방향으로 이동
    unsigned long releaseSteps = 0;
    while (digitalRead(limitPin) == LOW && releaseSteps < 5000) {
      digitalWrite(stepPin, HIGH);
      delayMicroseconds(10);
      digitalWrite(stepPin, LOW);
      
      if (stepDelay >= 1000) {
        delay(stepDelay / 1000);
      } else {
        delayMicroseconds(stepDelay);
      }
      
      releaseSteps++;
      
      if (releaseSteps % 50 == 0) {
        Serial.print("Release step "); Serial.print(releaseSteps);
        Serial.print(" - Limit: "); Serial.println(digitalRead(limitPin) == HIGH ? "HIGH" : "LOW");
      }
    }
    
    if (digitalRead(limitPin) == HIGH) {
      Serial.print("Limit switch released after "); Serial.print(releaseSteps); Serial.println(" steps.");
    } else {
      Serial.println("WARNING: Could not release limit switch!");
      return; // 홈잉 중단
    }
    
    // 추가로 100스텝 더 이동하여 여유 공간 확보
    Serial.println("Moving additional 100 steps for clearance...");
    for (int i = 0; i < 100; i++) {
      digitalWrite(stepPin, HIGH);
      delayMicroseconds(10);
      digitalWrite(stepPin, LOW);
      
      if (stepDelay >= 1000) {
        delay(stepDelay / 1000);
      } else {
        delayMicroseconds(stepDelay);
      }
    }
    
    delay(500); // 0.5초 대기
  }
  
  // 이제 정방향으로 리밋 스위치까지 이동
  Serial.println("Now moving towards limit switch...");
  
  // 정방향으로 설정
  digitalWrite(dirPin, direction > 0 ? HIGH : LOW);
  delay(10);
  
  unsigned long stepCount = 0;
  Serial.print("Moving "); Serial.print(axisName); Serial.print("-Axis towards limit... Current limit state: ");
  Serial.println(digitalRead(limitPin) == HIGH ? "HIGH (not triggered)" : "LOW (triggered)");
  
  while (digitalRead(limitPin) == HIGH) { // 리밋 스위치 감지까지 계속 이동
    // STEP 펄스 생성
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(10); // 펄스 폭 증가
    digitalWrite(stepPin, LOW);
    
    // 속도에 따른 딜레이 (더 안전하게)
    if (stepDelay >= 1000) {
      delay(stepDelay / 1000);
    } else {
      delayMicroseconds(stepDelay);
    }
    
    stepCount++;
    
    // 100스텝마다 상태 보고 (너무 많은 출력 방지)
    if (stepCount % 100 == 0) {
      Serial.print("Step "); Serial.print(stepCount); 
      Serial.print(" - Limit: "); Serial.println(digitalRead(limitPin) == HIGH ? "HIGH" : "LOW");
    }
    
    // 안전 장치: 최대 스텝 수 제한 (리밋 스위치 고장 시 무한 루프 방지)
    if (stepCount > 50000) {
      Serial.println("");
      Serial.print("WARNING: "); Serial.print(axisName); Serial.println("-Axis exceeded maximum steps! Check limit switch.");
      break;
    }
  }
  
  Serial.println("");
  if (digitalRead(limitPin) == LOW) {
    Serial.print(axisName); Serial.print("-Limit hit after "); Serial.print(stepCount); Serial.println(" steps.");
  } else {
    Serial.print(axisName); Serial.print("-Axis moved "); Serial.print(stepCount); Serial.println(" steps (limit not reached - test mode).");
  }
  
  // 백오프 (반대 방향으로 이동)
  if (digitalRead(limitPin) == LOW) {
    Serial.print("Backing off "); Serial.print(backoffPulses); Serial.println(" pulses...");
    
    // 반대 방향 설정
    digitalWrite(dirPin, direction > 0 ? LOW : HIGH);
    delay(10); // 방향 설정 안정화
    
    // 백오프 펄스 생성
    for (int i = 0; i < backoffPulses; i++) {
      digitalWrite(stepPin, HIGH);
      delayMicroseconds(10);
      digitalWrite(stepPin, LOW);
      if (stepDelay >= 1000) {
        delay(stepDelay / 1000);
      } else {
        delayMicroseconds(stepDelay);
      }
      
      if (i % 10 == 0) {
        Serial.print("Backoff step "); Serial.println(i + 1);
      }
    }
  }
  
  Serial.print(axisName); Serial.println("-Axis homing complete.");
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
