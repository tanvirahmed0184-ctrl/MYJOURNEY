// THIS IS YOUR ORIGINAL CODE (SAVED FOR REFERENCE)

// ----- PIN SETUP -----
#define lmf 2
#define lmb 6
#define lms 3
#define rmf 4
#define rmb 5
#define rms 9

// ----- VARIABLES -----
int s[6], sum;
int base[6]     = {1, 2, 4, 8, 16, 32};
int position[6] = {3, 2, 1, -1, -2, -3};
int threshold = 512;
int sensor_pos;

int lbase = 120;   // BASE SPEED LEFT
int rbase = 120;   // BASE SPEED RIGHT
int pwm_cap = 180; // MAX PWM

// PID
float avg = 0.0;
float PID = 0.0;
float error[2] = {0.0, 0.0};

// TURN & U-TURN STATE
char turn ='s';  // 's'=straight, 'l'=left, 'r'=right
char last_T_turn = 'l';   // alternate T-section turns
bool just_uturn = false;  // ignore T-section after U-turn

// MOTOR RAMP
int lmotor_actual = 0, rmotor_actual = 0;
int rate = 12; // max PWM change per loop

// PID GAINS
int kp = 40;
int kd = 100;

// TURN SPEED
int tsp = 80;

// ----- SETUP -----
void setup() {
  pinMode(lmf, OUTPUT);
  pinMode(lmb, OUTPUT);
  pinMode(rmf, OUTPUT);
  pinMode(rmb, OUTPUT);

  Serial.begin(9600); // debugging
}

// ----- MAIN LOOP -----
void loop() {
  semi_pid();
}

// ----- MOTOR FUNCTION -----
void motor(int a, int b){
  // CAP PWM
  a = constrain(a, -pwm_cap, pwm_cap);
  b = constrain(b, -pwm_cap, pwm_cap);

  // LEFT MOTOR
  if(a >= 0){
    digitalWrite(lmf, 1);
    digitalWrite(lmb, 0);
  } else {
    a = -a;
    digitalWrite(lmf, 0);
    digitalWrite(lmb, 1);
  }

  // RIGHT MOTOR
  if(b >= 0){
    digitalWrite(rmf, 1);
    digitalWrite(rmb, 0);
  } else {
    b = -b;
    digitalWrite(rmf, 0);
    digitalWrite(rmb, 1);
  }

  analogWrite(lms, a);
  analogWrite(rms, b);
}

// ----- SENSOR READ -----
void reading(){
  sensor_pos = 0;
  sum = 0;
  for(byte i=0; i<6; i++){
    s[i] = analogRead(i);
    s[i] = (s[i] > threshold) ? 1 : 0;
    sensor_pos += s[i] * position[i];
    sum += s[i];
  }
}

// ----- SEMI-PID FUNCTION -----
void semi_pid(){
  reading();

  // --- TURN DETECTION USING s0/s5 ---
  if(s[0] && !s[5]) turn = 'r';
  if(s[5] && !s[0]) turn = 'l';

  // --- DEAD-END / U-TURN DETECTION ---
  if(sum == 6){
    motor(0,0);
    delay(150);
    reading();
    if(sum == 0){ // dead-end detected
      do_uturn();
      just_uturn = true;
      return;
    }
  }

  // --- PID LINE FOLLOWING ---
  avg = (float)sensor_pos / max(1, (float)sum);
  error[0] = avg;
  float derivative = error[0] - error[1];
  PID = kp * error[0] + kd * derivative;
  error[1] = error[0];

  // clamp PID
  PID = constrain(PID, -120, 120);

  // target motor speeds
  int lmotor_target = constrain((int)(lbase + PID), 0, pwm_cap);
  int rmotor_target = constrain((int)(rbase - PID), 0, pwm_cap);

  // --- MOTOR RAMP ---
  if(lmotor_actual < lmotor_target) lmotor_actual += min(rate, lmotor_target - lmotor_actual);
  else if(lmotor_actual > lmotor_target) lmotor_actual -= min(rate, lmotor_actual - lmotor_actual);

  if(rmotor_actual < rmotor_target) rmotor_actual += min(rate, rmotor_target - rmotor_actual);
  else if(rmotor_actual > rmotor_target) rmotor_actual -= min(rate, rmotor_actual - rmotor_actual);

  motor(lmotor_actual, rmotor_actual);

  // --- T-SECTION / CROSS SECTION ---
  if(sum == 6 && !just_uturn){
    motor(0,0);
    delay(120);
    reading();
    if(sum != 0){ // T-section detected
      if(last_T_turn == 'l'){ do_turn_left(); last_T_turn='r'; }
      else { do_turn_right(); last_T_turn='l'; }
    }
  }

  // reset just_uturn when robot moves away
  if(just_uturn && sum != 6) just_uturn = false;

  // --- DEBUG ---
  Serial.print("S: ");
  for(int i=0;i<6;i++){ Serial.print(s[i]); Serial.print(" "); }
  Serial.print("| avg="); Serial.print(avg);
  Serial.print(" | PID="); Serial.print(PID);
  Serial.print(" | L="); Serial.print(lmotor_actual);
  Serial.print(" R="); Serial.println(rmotor_actual);
}

// ----- U-TURN / TURNS -----
void do_uturn(){
  motor(-120,120);
  delay(500);
  motor(0,0);
}

void do_turn_left(){
  motor(-120,120);
  delay(300);
  motor(0,0);
}

void do_turn_right(){
  motor(120,-120);
  delay(300);
  motor(0,0);
}
