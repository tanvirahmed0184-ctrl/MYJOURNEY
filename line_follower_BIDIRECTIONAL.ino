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

// TURN STATE
char last_T_turn = 'l';   // alternate T-section turns
bool just_uturn = false;  // ignore T-section after U-turn

// MOTOR RAMP
int lmotor_actual = 0, rmotor_actual = 0;
int rate = 12; // max PWM change per loop

// PID GAINS - ADJUSTED FOR BIDIRECTIONAL
int kp = 50;   // Increased for more aggressive corrections
int kd = 120;  // Increased for stability

// SHARP TURN PARAMETERS
int sharp_turn_forward_time = 150;
int sharp_turn_speed = 100;

// ----- SETUP -----
void setup() {
  pinMode(lmf, OUTPUT);
  pinMode(lmb, OUTPUT);
  pinMode(rmf, OUTPUT);
  pinMode(rmb, OUTPUT);

  Serial.begin(9600);
}

// ----- MAIN LOOP -----
void loop() {
  semi_pid();
}

// ----- MOTOR FUNCTION -----
void motor(int a, int b){
  // CAP PWM (NOW BIDIRECTIONAL!)
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

// ----- SEMI-PID FUNCTION (BIDIRECTIONAL VERSION) -----
void semi_pid(){
  reading();

  // --- JUNCTION DETECTION ---
  if(sum == 6){
    motor(0, 0);
    delay(150);
    reading();
    
    if(sum == 0){ 
      // DEAD-END
      do_uturn();
      just_uturn = true;
      return;
    } 
    else if(sum >= 3 && !just_uturn){ 
      // T-SECTION
      if(last_T_turn == 'l'){ 
        do_turn_left(); 
        last_T_turn = 'r'; 
      } else { 
        do_turn_right(); 
        last_T_turn = 'l'; 
      }
      return;
    }
  }

  // --- SHARP TURN DETECTION ---
  if(s[0] && !s[1] && !s[2] && !s[3] && !s[4] && !s[5]){
    motor(lbase, rbase);
    delay(sharp_turn_forward_time);
    do_sharp_turn_right();
    return;
  }
  
  if(s[5] && !s[4] && !s[3] && !s[2] && !s[1] && !s[0]){
    motor(lbase, rbase);
    delay(sharp_turn_forward_time);
    do_sharp_turn_left();
    return;
  }

  // --- NORMAL PID LINE FOLLOWING (BIDIRECTIONAL!) ---
  if(sum > 0){
    avg = (float)sensor_pos / (float)sum;
    error[0] = avg;
    float derivative = error[0] - error[1];
    PID = kp * error[0] + kd * derivative;
    error[1] = error[0];

    // clamp PID (wider range for bidirectional)
    PID = constrain(PID, -150, 150);

    // *** KEY CHANGE: ALLOW NEGATIVE VALUES! ***
    // This enables pivot turns (one wheel forward, one reverse)
    int lmotor_target = constrain((int)(lbase + PID), -pwm_cap, pwm_cap);
    int rmotor_target = constrain((int)(rbase - PID), -pwm_cap, pwm_cap);

    // --- MOTOR RAMP ---
    if(lmotor_actual < lmotor_target) {
      lmotor_actual += min(rate, lmotor_target - lmotor_actual);
    } else if(lmotor_actual > lmotor_target) {
      lmotor_actual -= min(rate, lmotor_actual - lmotor_target);
    }

    if(rmotor_actual < rmotor_target) {
      rmotor_actual += min(rate, rmotor_target - rmotor_actual);
    } else if(rmotor_actual > rmotor_target) {
      rmotor_actual -= min(rate, rmotor_actual - rmotor_target);
    }

    motor(lmotor_actual, rmotor_actual);
  } 
  else {
    // Lost line - continue last direction
    motor(lmotor_actual, rmotor_actual);
  }

  // Reset just_uturn flag
  if(just_uturn && sum < 6 && sum > 0) {
    just_uturn = false;
  }

  // --- DEBUG ---
  Serial.print("S: ");
  for(int i=0; i<6; i++){ Serial.print(s[i]); Serial.print(" "); }
  Serial.print("| sum="); Serial.print(sum);
  Serial.print(" | avg="); Serial.print(avg);
  Serial.print(" | PID="); Serial.print(PID);
  Serial.print(" | L="); Serial.print(lmotor_actual);
  Serial.print(" R="); Serial.println(rmotor_actual);
}

// ----- U-TURN -----
void do_uturn(){
  Serial.println("U-TURN!");
  motor(-120, 120);
  delay(600);
  
  reading();
  while(sum == 0 || sum > 4){
    motor(-120, 120);
    delay(50);
    reading();
  }
  motor(0, 0);
  delay(100);
}

// ----- T-SECTION TURNS -----
void do_turn_left(){
  Serial.println("T-TURN LEFT");
  motor(-100, 100);
  delay(350);
  
  reading();
  while(!(s[2] || s[3])){
    motor(-100, 100);
    delay(30);
    reading();
  }
  motor(0, 0);
  delay(50);
}

void do_turn_right(){
  Serial.println("T-TURN RIGHT");
  motor(100, -100);
  delay(350);
  
  reading();
  while(!(s[2] || s[3])){
    motor(100, -100);
    delay(30);
    reading();
  }
  motor(0, 0);
  delay(50);
}

// ----- SHARP TURNS -----
void do_sharp_turn_left(){
  Serial.println("SHARP LEFT");
  motor(-sharp_turn_speed, sharp_turn_speed);
  
  reading();
  while(sum == 0 || sum == 1){
    motor(-sharp_turn_speed, sharp_turn_speed);
    delay(30);
    reading();
  }
  motor(0, 0);
  delay(50);
}

void do_sharp_turn_right(){
  Serial.println("SHARP RIGHT");
  motor(sharp_turn_speed, -sharp_turn_speed);
  
  reading();
  while(sum == 0 || sum == 1){
    motor(sharp_turn_speed, -sharp_turn_speed);
    delay(30);
    reading();
  }
  motor(0, 0);
  delay(50);
}
