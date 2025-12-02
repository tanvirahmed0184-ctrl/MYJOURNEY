// ==============================
// FIXED LFR - Based on working code logic
// Key fixes:
// 1. Proper no-line handling (use last_error like working code)
// 2. Simplified sharp turn detection
// 3. Removed problematic motor ramping
// 4. Cleaner junction handling
// ==============================

#define lmf 2
#define lmb 6
#define lms 3
#define rmf 4
#define rmb 5
#define rms 9

int s[6], sum;
int base[6] = {1, 2, 4, 8, 16, 32};
int position[6] = {3, 2, 1, -1, -2, -3}; // s[0]=rightmost, s[5]=leftmost
int threshold = 200;
int sensor_pos;
int lbase = 140;
int rbase = 140;
int pwm_cap = 180;

float avg = 0.0;
float PID = 0.0;
float error[2] = {0.0, 0.0};
float last_avg = 0.0; // CRITICAL: store last valid avg for no-line case

char turn = 's';
char last_turn = 'r';
char last_T_turn = 'l';
bool just_junction = false;

int kp = 35;
int kd = 60;
int sharp_turn_forward_time = 100;
int sharp_turn_speed = 100;

bool debug_mode = true;
bool DRY_RUN = true; // Set false to run motors
bool SAFE_MOTOR_OUTPUT = true;

void setup(){
  pinMode(lmf, OUTPUT);
  pinMode(lmb, OUTPUT);
  pinMode(rmf, OUTPUT);
  pinMode(rmb, OUTPUT);
  Serial.begin(9600);
  delay(300);
  Serial.println("=== FIXED LFR DEBUG ===");
  Serial.print("DRY_RUN = ");
  Serial.println(DRY_RUN ? "true" : "false");
  Serial.println("================================");
}

void motor_raw_drive(int a, int b){
  a = constrain(a, -pwm_cap, pwm_cap);
  b = constrain(b, -pwm_cap, pwm_cap);
  
  // left motor
  if(a >= 0){
    digitalWrite(lmf, HIGH);
    digitalWrite(lmb, LOW);
  } else {
    a = -a;
    digitalWrite(lmf, LOW);
    digitalWrite(lmb, HIGH);
  }
  
  // right motor
  if(b >= 0){
    digitalWrite(rmf, HIGH);
    digitalWrite(rmb, LOW);
  } else {
    b = -b;
    digitalWrite(rmf, LOW);
    digitalWrite(rmb, HIGH);
  }
  
  analogWrite(lms, a);
  analogWrite(rms, b);
}

void motor(int a, int b){
  if(debug_mode){
    Serial.print("[CMD motor] L=");
    Serial.print(a);
    Serial.print(" R=");
    Serial.println(b);
  }
  
  if(DRY_RUN){
    return;
  } else {
    if(SAFE_MOTOR_OUTPUT){
      int safe_cap = 90;
      a = constrain(a, -safe_cap, safe_cap);
      b = constrain(b, -safe_cap, safe_cap);
    }
    motor_raw_drive(a, b);
  }
}

void reading_raw_and_bin(){
  sensor_pos = 0;
  sum = 0;
  Serial.print("RAW: ");
  for(int i=0; i<6; i++){
    int raw = analogRead(i);
    Serial.print(raw);
    Serial.print(" ");
    s[i] = (raw > threshold) ? 1 : 0;
    sensor_pos += s[i] * position[i];
    sum += s[i];
  }
  Serial.print(" | BIN: ");
  for(int i=0; i<6; i++){
    Serial.print(s[i]);
    Serial.print(" ");
  }
  Serial.print(" | sum=");
  Serial.print(sum);
  Serial.print(" | sensor_pos=");
  Serial.print(sensor_pos);
}

void reading(){
  sensor_pos = 0;
  sum = 0;
  for(byte i=0; i<6; i++){
    int raw = analogRead(i);
    s[i] = (raw > threshold) ? 1 : 0;
    sensor_pos += s[i] * position[i];
    sum += s[i];
  }
}

void semi_pid(){
  reading_raw_and_bin();
  
  // CRITICAL FIX: Calculate avg properly and store for no-line case
  if(sum > 0){
    avg = (float)sensor_pos / (float)sum;
    last_avg = avg; // Store valid avg
  } else {
    // NO LINE: Use last valid avg (like working code uses last_error)
    avg = last_avg;
    Serial.print(" | NO LINE -> using last_avg=");
    Serial.print(avg);
  }
  
  Serial.print(" | avg=");
  Serial.print(avg);
  Serial.print(" | last_turn=");
  Serial.print(last_turn);
  Serial.print(" | just_junction=");
  Serial.print(just_junction);
  
  // Update turn memory
  turn = 's';
  if (s[0] && !s[5]) {
    turn = 'r';
    last_turn = 'r';
  } else if (s[5] && !s[0]) {
    turn = 'l';
    last_turn = 'l';
  }
  Serial.print(" | turn=");
  Serial.print(turn);
  
  // Junction detection
  if(sum == 6 && !just_junction){
    Serial.print(" --> DETECT sum==6 branch");
    motor(0, 0);
    delay(100);
    reading();
    Serial.print(" re-sum=");
    Serial.print(sum);
    
    if(sum == 6){
      Serial.println(" STILL 6 -> black wall STOP");
      motor(0, 0);
      if(DRY_RUN){
        Serial.println("DRY_RUN: not entering infinite stop");
        just_junction = true;
        return;
      } else {
        Serial.println("Stopping (black wall) - motors off");
        motor(0, 0);
        while(1);
      }
    } else if(sum >= 3){
      if(s[2] || s[3]){
        Serial.println(" CROSS detected -> going straight");
        motor(lbase, rbase);
        delay(200);
        motor(0, 0);
      } else {
        Serial.println(" T-section detected -> do_turn");
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
  
  // FIXED: More conservative sharp turn detection
  // Only trigger if EXTREME edge case (one edge sensor AND very few sensors)
  if(sum <= 1 && ((s[0] && !s[5]) || (s[5] && !s[0]))){
    Serial.println(" --> sharp-turn branch triggered");
    bool turn_right = s[0];
    motor(lbase, rbase);
    delay(sharp_turn_forward_time);
    if(turn_right){
      Serial.println(" DO sharp right");
      do_sharp_turn_right();
    } else {
      Serial.println(" DO sharp left");
      do_sharp_turn_left();
    }
    return;
  }
  
  // CRITICAL FIX: Always use PID (even when sum==0, avg uses last_avg)
  // This ensures balanced motor outputs
  error[0] = avg;
  float derivative = error[0] - error[1];
  PID = kp * error[0] + kd * derivative;
  error[1] = error[0];
  PID = constrain(PID, -150, 150);
  
  int lmotor_target = constrain((int)(lbase + PID), -pwm_cap, pwm_cap);
  int rmotor_target = constrain((int)(rbase - PID), -pwm_cap, pwm_cap);
  
  Serial.print(" | PID=");
  Serial.print(PID);
  Serial.print(" L_t=");
  Serial.print(lmotor_target);
  Serial.print(" R_t=");
  Serial.println(rmotor_target);
  
  // CRITICAL FIX: Direct motor control (no ramping that accumulates errors)
  motor(lmotor_target, rmotor_target);
  
  if(just_junction && sum < 6) just_junction = false;
  
  Serial.println();
}

void do_turn_left(){
  Serial.println(">>> do_turn_left()");
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
  Serial.println("<<< left turn done");
}

void do_turn_right(){
  Serial.println(">>> do_turn_right()");
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
  Serial.println("<<< right turn done");
}

void do_sharp_turn_left(){
  Serial.println(">>> do_sharp_turn_left()");
  motor(-sharp_turn_speed, sharp_turn_speed);
  reading();
  while(sum == 0 || sum == 1){
    motor(-sharp_turn_speed, sharp_turn_speed);
    delay(30);
    reading();
  }
  motor(0, 0);
  delay(50);
  Serial.println("<<< sharp left done");
}

void do_sharp_turn_right(){
  Serial.println(">>> do_sharp_turn_right()");
  motor(sharp_turn_speed, -sharp_turn_speed);
  reading();
  while(sum == 0 || sum == 1){
    motor(sharp_turn_speed, -sharp_turn_speed);
    delay(30);
    reading();
  }
  motor(0, 0);
  delay(50);
  Serial.println("<<< sharp right done");
}

void loop(){
  semi_pid();
  delay(600);
}
