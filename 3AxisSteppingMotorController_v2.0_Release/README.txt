=====================================================
3축 스테핑 모터 제어기 v2.0
3-Axis Stepping Motor Controller v2.0
=====================================================

이 패키지는 3축 스테핑 모터를 제어하기 위한 완전한 솔루션을 제공합니다.

[패키지 구성]
1. 3AxisSteppingMotorController.exe - 메인 GUI 애플리케이션
2. 3AxisSteppingMotorControl_arduino/ - Arduino 소스 코드 폴더

[시스템 요구사항]
- Windows 10 이상
- Arduino Uno 또는 호환 보드
- 스테핑 모터 드라이버 (A4988, DRV8825 등)
- 3축 스테핑 모터
- 리밋 스위치 (선택사항)

[Arduino 설정 방법]
1. Arduino IDE를 설치합니다. (https://www.arduino.cc/en/software)
2. AccelStepper 라이브러리를 설치합니다:
   - Arduino IDE에서 [스케치] → [라이브러리 포함하기] → [라이브러리 관리...]
   - "AccelStepper"를 검색하여 설치
3. 3AxisSteppingMotorControl_arduino 폴더의 .ino 파일을 Arduino IDE로 엽니다.
4. Arduino 보드에 업로드합니다.

[하드웨어 연결]
Arduino Uno 핀 배치:
- X축: STEP=2, DIR=5, LIMIT=9
- Y축: STEP=3, DIR=6, LIMIT=10  
- Z축: STEP=4, DIR=7, LIMIT=11

드라이버 연결:
- 각 축의 STEP, DIR 핀을 해당 Arduino 핀에 연결
- 드라이버의 GND를 Arduino GND에 연결
- 모터 드라이버에 적절한 전원 공급 (보통 12V 또는 24V)
- ENABLE 핀이 있는 경우 LOW로 설정하거나 GND에 연결

리밋 스위치 연결 (선택사항):
- 각 축의 리밋 스위치를 해당 LIMIT 핀에 연결
- 스위치가 눌리지 않았을 때 HIGH, 눌렸을 때 LOW가 되도록 연결
- 풀업 저항 사용 권장 (10kΩ)

[소프트웨어 사용 방법]
1. 3AxisSteppingMotorController.exe를 실행합니다.
2. Settings 탭에서 COM 포트를 설정합니다.
3. Connect 버튼을 클릭하여 Arduino와 연결합니다.
4. Control 탭에서 각 축을 개별적으로 제어할 수 있습니다.

[주요 기능]
- 개별 축 제어 (X, Y, Z)
- 홈잉 기능 (원점 복귀)
- 방향 반전 옵션
- 속도 및 가속도 조절
- 설정 저장/불러오기
- 리밋 스위치 지원

[Setting Mode 기능]
- Max Position Settings: 각 축의 최대 이동 범위 설정
- Homing Speed: 홈잉 시 이동 속도 설정 (steps/sec)
- Backoff Pulses: 홈잉 후 백오프 거리 설정

[주의사항]
1. 하드웨어 연결을 확인한 후 소프트웨어를 실행하세요.
2. 처음 사용 시 낮은 속도로 테스트해보세요.
3. 리밋 스위치가 없는 경우 수동으로 이동 범위를 제한하세요.
4. 모터가 과열되지 않도록 주의하세요.
5. 전원 공급이 충분한지 확인하세요.

[문제 해결]
- 모터가 소리만 나고 돌지 않는 경우:
  * 드라이버 연결 확인
  * 전원 공급 확인
  * ENABLE 핀 설정 확인
  * 펄스 폭 설정 확인

- 통신 오류가 발생하는 경우:
  * COM 포트 설정 확인
  * Arduino 연결 상태 확인
  * 다른 프로그램에서 COM 포트 사용 여부 확인

[업데이트 내역 v2.0]
- 직접 펄스 생성 방식으로 변경하여 정확한 속도 제어
- 홈잉 시작 시 리밋 스위치 상태 확인 및 자동 해제 기능 추가
- 개선된 디버깅 정보 출력
- 안정성 향상

[연락처]
문제가 발생하거나 질문이 있으시면 관련 문서를 참조하시거나 
Arduino 및 스테핑 모터 제어 관련 커뮤니티에 문의하세요.

=====================================================
© 2024 3-Axis Stepping Motor Controller
===================================================== 