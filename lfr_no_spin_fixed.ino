// ADVANCED BIDIRECTIONAL LINE FOLLOWER - FIXED VERSION (NO SPIN)
// Fixes spinning issue by removing problematic lost-line pivoting
// Date: Dec 2, 2025

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
int position[6] = {3, 2, 1, -1, -2, -3};  // s[0]=rightmost, s[5]=leftmost
int threshold = 200;
int sensor_pos;

int lbase = 120;   // BASE SPEED LEFT
int rbase = 120;   // BASE SPEED RIGHT
int pwm_cap = 180; // MAX PWM

// PID
float avg = 0.0;
float PID = 0.0;
float error[2] = {0.0, 0.0};
float last_avg = 0.0;  // CRITICAL FIX: Store last valid avg for no-line case

// TURN STATE & MEMORY
char turn = 's';         // current reading (reset each loop)
char last_turn = 'r';    // Initialize to 'r'
char last_T_turn = 'l';  // alternate T-section turns
bool just_junction = false;

// PID GAINS
int kp = 40;   // Proportional gain
int kd = 100;  // Derivative gain

// TURN PARAMETERS
int sharp_turn_forward_time = 100;
int sharp_turn_speed = 100;

// DEBUG MODE (set to false for competition to improve speed)
bool debug_mode = true;

// ----- SETUP -----
void setup() {
  pinMode(lmf, OUTPUT);
  pinMode(lmb, OUTPUT);
  pinMode(rmf, OUTPUT);
  pinMode(rmb, OUTPUT);

  Serial.begin(9600);
  
  if(debug_mode){
    Serial.println("=== LINE FOLLOWER INITIALIZED ===");
    Serial.println("Version: Fixed (No Spin - Removed Lost Line Pivoting)");
    Serial.print("Threshold: "); Serial.println(threshold);
    Serial.print("Base Speed: L="); Serial.print(lbase);
    Serial.print(" R="); Serial.println(rbase);
    Serial.println("================================");
  }
}

// ----- MAIN LOOP -----
void loop() {
  semi_pid();
  delay(5);  // Small delay to prevent too fast execution
}

// ----- MOTOR FUNCTION -----
void motor(int a, int b){
  // BIDIRECTIONAL: -pwm_cap to +pwm_cap
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

// ----- SEMI-PID FUNCTION (FIXED - NO SPIN) -----
void semi_pid(){
  reading();
  
  // RESET current turn each loop
  turn = 's';

  // UPDATE BASED ON EDGE SENSORS (update memory only when bias seen)
  if (s[0] && !s[5]) {
    turn = 'r';
    last_turn = 'r';
  } else if (s[5] && !s[0]) {
    turn = 'l';
    last_turn = 'l';
  }

  // CRITICAL FIX: Calculate avg and store for no-line case
  if(sum > 0){
    avg = (float)sensor_pos / (float)sum;
    last_avg = avg;  // Store valid avg
  } else {
    // NO LINE: Use last valid avg (like working code)
    // This prevents spinning by maintaining last direction
    avg = last_avg;
    if(debug_mode){
      Serial.print("NO LINE -> using last_avg=");
      Serial.print(avg);
      Serial.print(" ");
    }
  }

  // --- JUNCTION DETECTION ---
  if(sum == 6 && !just_junction){
    motor(0, 0);
    delay(100);
    reading();
    
    if(sum == 6){
      // STILL ALL BLACK = BLACK WALL / END
      if(debug_mode) Serial.println("BLACK WALL - END! STOPPING.");
      motor(0, 0);
      while(1); // Stop forever
    }
    else if(sum >= 3){
      // Could be T-section OR cross junction
      if(s[2] || s[3]){
        // CROSS JUNCTION - GO STRAIGHT
        if(debug_mode) Serial.println("CROSS JUNCTION - GOING STRAIGHT");
        motor(lbase, rbase);
        delay(200);
      }
      else {
        // T-SECTION - ALTERNATE TURN
        if(last_T_turn == 'l'){ 
          do_turn_left(); 
          last_T_turn = 'r'; 
        } else { 
          do_turn_right(); 
          last_T_turn = 'l'; 
        }
      }
      just_junction = true;
      return;
    }
  }

  // --- IMPROVED SHARP TURN DETECTION (more conservative) ---
  if(sum <= 1 && ((s[0] && !s[5]) || (s[5] && !s[0]))){
    bool turn_right = s[0];
    
    motor(lbase, rbase);
    delay(sharp_turn_forward_time);
    
    if(turn_right){
      if(debug_mode) Serial.println("SHARP RIGHT");
      do_sharp_turn_right();
    } else {
      if(debug_mode) Serial.println("SHARP LEFT");
      do_sharp_turn_left();
    }
    return;
  }

  // --- NORMAL PID LINE FOLLOWING (ALWAYS RUNS, EVEN WHEN sum==0) ---
  // CRITICAL FIX: Always calculate PID using avg (which uses last_avg when sum==0)
  // This ensures balanced motor outputs and prevents spinning
  error[0] = avg;
  float derivative = error[0] - error[1];
  PID = kp * error[0] + kd * derivative;
  error[1] = error[0];

  // Wider PID range for bidirectional
  PID = constrain(PID, -150, 150);

  // BIDIRECTIONAL: ALLOW NEGATIVE VALUES
  int lmotor_target = constrain((int)(lbase + PID), -pwm_cap, pwm_cap);
  int rmotor_target = constrain((int)(rbase - PID), -pwm_cap, pwm_cap);

  // CRITICAL FIX: Direct motor control (no ramping that can accumulate errors)
  motor(lmotor_target, rmotor_target);
  
  if(just_junction && sum < 6) just_junction = false;

  // DEBUG (only if enabled)
  if(debug_mode){
    Serial.print("S: ");
    for(int i=0; i<6; i++){ Serial.print(s[i]); Serial.print(" "); }
    Serial.print("| sum="); Serial.print(sum);
    Serial.print(" | turn="); Serial.print(turn);
    Serial.print(" | avg="); Serial.print(avg);
    Serial.print(" | PID="); Serial.print(PID);
    Serial.print(" | L="); Serial.print(lmotor_target);
    Serial.print(" R="); Serial.println(rmotor_target);
  }
}

// ----- T-SECTION TURNS -----
void do_turn_left(){
  if(debug_mode) Serial.println("T-TURN LEFT");
  motor(-100, 100);
  delay(250);
  
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
  if(debug_mode) Serial.println("T-TURN RIGHT");
  motor(100, -100);
  delay(250);
  
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
