#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

#define PREP_TIME 50
#define N_SERVOS 6
const int SERVO_PINS[N_SERVOS] = {10,11,12,13,14,15};

// const int DEG_UP[N_SERVOS] = {31,27,135,144,137,0};
// const int DEG_DO[N_SERVOS] = {144,131,0,31,27,135};

const int DEG_UP[N_SERVOS] = {41,37,125,134,127,10};
const int DEG_DO[N_SERVOS] = {81,77, 85, 94, 87,50};

int PULSE[2 * N_SERVOS] = {0};
int servo_state[N_SERVOS] = {1};

#define MINPULSE 150 // This is the 'minimum' pulse length count (out of 4096)
#define MAXPULSE 600 // This is the 'maximum' pulse length count (out of 4096)
#define SERVO_FREQ 50 // Analog servos run at ~50 Hz updates
#define OSC_FREQ 24890000 // Specific to each PCA 9685 board

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

bool hand_shaked = false;
#define BUF_SIZE 64
int buf_push_i = 0;
int buf_pop_i = 0;
int buf_time[BUF_SIZE] = {0};
int buf_pos [BUF_SIZE] = {0};

void setup() {
  pwm.begin();
  pwm.setOscillatorFrequency(OSC_FREQ);
  pwm.setPWMFreq(SERVO_FREQ);
  for (int i = 0; i < N_SERVOS; i ++) {
    PULSE[           i] = map(DEG_UP[i], 0, 180, MINPULSE, MAXPULSE);
    PULSE[N_SERVOS + i] = map(DEG_DO[i], 0, 180, MINPULSE, MAXPULSE);
  }

  for (int i = 0; i < N_SERVOS; i++) {
    servo_state[i] = 1;
    toggleServo(i);
  }
//   delay(1000);
//   for (int i = 0; i < N_SERVOS; i++) {
//     toggleServo(i);
//   }

  Serial.begin(9600);
  handShake();
}

void handShake() {
  char* expecting = "hi arduino";
  int len = 10; // exclude the terminating \x00
  for (int i = 0; i < len; i++) {
    if (readOne() != expecting[i]) {
      fatalError("Handshake failed! ");
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

bool loaded = false;
bool song_ended = false;
long start_time = 0;
void loop() {
  while (! loaded) {
    song_ended = false;
    load();
    Serial.write('s');
    readOne();
    start_time = millis();
  }
  int note_time = buf_time[buf_pop_i];
  int t = millis() - start_time;
  int dt = note_time - t;
  if (dt <= PREP_TIME) {
    // note
    int pos = buf_pos[buf_pop_i];
    accCircularPointer(& buf_pop_i);
    // debugPrint("pop_i=", buf_pop_i);
    static const int FINGER_MAP[4] = {1, 0, 3, 4};
    int servo_id = FINGER_MAP[pos];
    t = millis() - start_time;  // update time estimates
    dt = note_time - t;
    if (dt > 0) {
      delay(dt);
    }
    toggleServo(servo_id);
    if (buf_pop_i == buf_push_i) {
      if (song_ended) {
        loaded = false;
      } else {
        fatalError("buf underflow");
      }
    }
  } else {
    if (! song_ended && bufAvailable()) {
      request();
    }
  }
}

int bufAvailable() {
  int x = buf_pop_i - buf_push_i;
  if (x == 1) return false;
  if (x == 1 - BUF_SIZE) return false;
  return true;
}

void accCircularPointer(int* x) {
  (*x) ++;
  if (*x == BUF_SIZE) {
    (*x) -= BUF_SIZE;
  }
}

void load() {
  loaded = true;
  buf_push_i = 0;
  for (int _ = 0; _ < BUF_SIZE - 1; _ ++) {
    request();
    if (! loaded) {
      return;
    }
  }
  buf_pop_i = 0;
}

void request() {
  Serial.write('r');
  char head = readOne();
  switch (head) {
    case '!':
      loaded = false;
      break;
    case 'n':
      static char* time_bytes = new char[4];
      Serial.readBytes(time_bytes, 3);
      buf_time[buf_push_i] = decodeTime(time_bytes);
      buf_pos [buf_push_i] = decodeDigit(readOne());
      accCircularPointer(& buf_push_i);
      // debugPrint("push_i=", buf_push_i);
      break;
    case 'e':
      song_ended = true;
      break;
    default:
      fatalError((char*) ("Unexpected header: " + head));
  }
}

void fatalError(char* msg) {
  while (true) {
    Serial.println(msg);
    delay(1000);
  }
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

void debugPrint(String k, int v) {
  Serial.write('d');
  Serial.print(k);
  String _v = String(v);
  Serial.print(_v);
  for (int i = 16 - k.length() - _v.length(); i > 0; i --) {
    Serial.write('_');
  }
}
