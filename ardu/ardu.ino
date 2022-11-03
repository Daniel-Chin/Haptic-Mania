#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

#define N_SERVOS 6
const int SERVO_PINS[N_SERVOS] = {10,11,12,13,14,15};
const int DEG_UP[N_SERVOS] = {31,27,135,144,137,0};
const int DEG_DO[N_SERVOS] = {144,131,0,31,27,135};
int PULSE[2 * N_SERVOS] = {0};
byte servo_state[N_SERVOS] = {1};

#define MINPULSE 150 // This is the 'minimum' pulse length count (out of 4096)
#define MAXPULSE 600 // This is the 'maximum' pulse length count (out of 4096)
#define SERVO_FREQ 50 // Analog servos run at ~50 Hz updates
#define OSC_FREQ 24890000 // Specific to each PCA 9685 board

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

bool hand_shaked = false;
#define BUF_SIZE 64
byte buf_pop_i  = 0;
byte buf_push_i = 0;
int  buf_time[BUF_SIZE] = {0};
byte buf_pos [BUF_SIZE] = {0};

void setup() {
  pwm.begin();
  pwm.setOscillatorFrequency(OSC_FREQ);
  pwm.setPWMFreq(SERVO_FREQ);
  for (int i = 0; i < N_SERVOS; i ++) {
    PULSE[           i] = map(DEG_UP[i], 0, 180, MINPULSE, MAXPULSE);
    PULSE[N_SERVOS + i] = map(DEG_DO[i], 0, 180, MINPULSE, MAXPULSE);
  }

  for (int i = 0; i < N_SERVOS; i++) {
    toggleServo(i);
  }

  Serial.begin(9600);
  handShake();
}

void handShake() {
  String expecting = "hi arduino";
  int len = expecting.length();
  for (int i = 0; i < len; i++) {
    if (readOne() != expecting[i]) {
      _abort = true;
      fatalError("Handshake failed! ");
      return;
    }
  }
  Serial.print("hi python");
  hand_shaked = true;
}

char readOne() {    // blocking
  int recved = -1;
  while (recved == -1) {
    recved = Serial.read();
  }
  return (char) recved;
}

void loop() {
}

void mySerialEvent() {
  if (! hand_shaked) return;

}

int decodeDigit(char x) {
  return (int) x - (int) '0';
}

int decodeTime(char* x) {
  return (
    +          ((int) (* x     )) - 1
    + 126   * (((int) (*(x + 1))) - 1)
    + 15876 * (((int) (*(x + 2))) - 1)
  );
}

void toggleServo(int id) {
  servo_state[id] = 1 - servo_state[id];
  pwm.setPWM(SERVO_PINS[id], 0, PULSE[
    servo_state[id] * N_SERVOS + id
  ]);
}
