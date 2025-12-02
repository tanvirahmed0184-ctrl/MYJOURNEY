// ADVANCED BIDIRECTIONAL LINE FOLLOWER - FIXED VERSION
// Fixes cross junction bug and adds improvements
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
int threshold = 512;
int sensor_pos;

int lbase = 120;   // BASE SPEED LEFT
int rbase = 120;   // BASE SPEED RIGHT
int pwm_cap = 180; // MAX PWM

// PID
float avg = 0.0;
float PID = 0.0;
float error[2] = {0.0, 0.0};

// TURN STATE & MEMORY
char turn = 's';         // current reading (reset each loop)
char last_turn = 'r';    // FIX: Initialize to 'r' instead of 's'
char last_T_turn = 'l';  // alternate T-section turns
bool just_junction = false;

// MOTOR RAMP
int lmotor_actual = 0, rmotor_actual = 0;
int rate = 12;

// PID GAINS
int kp = 40;   // Proportional gain
int kd = 100;  // Derivative gain

// TURN PARAMETERS
int sharp_turn_forward_time = 100;  // FIXED: Was 10ms - way too short!
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
  
  // TEMPORARY: Print sensor values for 3 seconds
  Serial.println("=== SENSOR CALIBRATION ===");
  Serial.println("Place robot on WHITE surface first, then on BLACK line");
  Serial.println("Watch the values - WHITE should be LOW, BLACK should be HIGH");
  Serial.print("Threshold is currently: ");
  Serial.println(threshold);
  Serial.println("Starting calibration in 2 seconds...");
  delay(2000);
  
  for(int i=0; i<30; i++){
    Serial.print("Sensors: ");
    for(int j=0; j<6; j++){
      Serial.print(analogRead(j));
      Serial.print(" ");
    }
    Serial.println();
    delay(100);
  }
  Serial.println("=== END CALIBRATION ===");
  Serial.println("If WHITE values are > 512, threshold is too LOW");
  Serial.println("If BLACK values are < 512, threshold is too HIGH");
  Serial.println("Adjust threshold to be between WHITE and BLACK values");
  delay(2000);
  
  if(debug_mode){
    Serial.println("=== LINE FOLLOWER INITIALIZED ===");
    Serial.println("Version: Fixed (Cross Junction Support)");
    Serial.print("Threshold: "); Serial.println(threshold);
    Serial.print("Base Speed: L="); Serial.print(lbase);
    Serial.print(" R="); Serial.println(rbase);
    Serial.println("================================");
  }
}

// ----- MAIN LOOP -----
void loop() {
  semi_pid();
  delay(5);  // Small delay to prevent too fast execution and sensor reading issues
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

// ----- SEMI-PID FUNCTION (IMPROVED + FIXED) -----
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
  // otherwise leave last_turn unchanged (memory preserved)

  // --- JUNCTION DETECTION ---
  if(sum == 6 && !just_junction){
    // Move forward a bit to get past the junction
    motor(lbase, rbase);
    delay(150);  // Move forward to get past junction
    reading();
    
    if(sum == 6){
      // STILL ALL BLACK = BLACK WALL / END
      if(debug_mode) Serial.println("BLACK WALL - END! STOPPING.");
      motor(0, 0);
      while(1); // Stop forever
    }
    else if(s[2] || s[3]){
      // MIDDLE SENSORS SEE LINE = CROSS JUNCTION - GO STRAIGHT
      if(debug_mode) Serial.println("CROSS JUNCTION - GOING STRAIGHT");
      motor(lbase, rbase);
      delay(200);  // Continue straight through cross junction
    }
    else if(sum == 0){
      // ALL WHITE = T-SECTION - ALTERNATE TURN
      if(debug_mode) Serial.println("T-SECTION DETECTED");
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

  // --- IMPROVED SHARP TURN DETECTION ---
  if((s[0] && sum <= 2 && !s[5]) || (s[5] && sum <= 2 && !s[0])){
    bool turn_right = s[0];
    
    motor(lbase, rbase);
    delay(sharp_turn_forward_time);
    
    if(turn_right){
      if(debug_mode) Serial.println("SHARP RIGHT (sum<=2)");
      do_sharp_turn_right();
    } else {
      if(debug_mode) Serial.println("SHARP LEFT (sum<=2)");
      do_sharp_turn_left();
    }
    return;
  }

  // --- NORMAL PID LINE FOLLOWING (BIDIRECTIONAL) ---
  if(sum > 0){
    avg = (float)sensor_pos / (float)sum;
    error[0] = avg;
    float derivative = error[0] - error[1];
    PID = kp * error[0] + kd * derivative;
    error[1] = error[0];

    // Wider PID range for bidirectional
    PID = constrain(PID, -150, 150);

    // BIDIRECTIONAL: ALLOW NEGATIVE VALUES
    int lmotor_target = constrain((int)(lbase + PID), -pwm_cap, pwm_cap);
    int rmotor_target = constrain((int)(rbase - PID), -pwm_cap, pwm_cap);

    // MOTOR RAMP
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
    
    if(just_junction && sum < 6) just_junction = false;
  } 
  else {
    // NO LINE DETECTED - Continue with last motor speeds
    // PID will naturally recover when sensors see line again
    motor(lmotor_actual, rmotor_actual);
    
    if(debug_mode){
      Serial.println("NO LINE - continuing with last speeds");
    }
  }

  // DEBUG (only if enabled)
  if(debug_mode){
    Serial.print("S: ");
    for(int i=0; i<6; i++){ Serial.print(s[i]); Serial.print(" "); }
    Serial.print("| sum="); Serial.print(sum);
    Serial.print(" | turn="); Serial.print(turn);
    Serial.print(" | avg="); Serial.print(avg);
    Serial.print(" | PID="); Serial.print(PID);
    Serial.print(" | L="); Serial.print(lmotor_actual);
    Serial.print(" R="); Serial.println(rmotor_actual);
    
    // Every 50 loops, print raw sensor values to check threshold
    static int debug_counter = 0;
    debug_counter++;
    if(debug_counter >= 50){
      Serial.print("RAW SENSORS: ");
      for(int i=0; i<6; i++){
        Serial.print(analogRead(i));
        Serial.print(" ");
      }
      Serial.print("| Threshold=");
      Serial.println(threshold);
      debug_counter = 0;
    }
  }
}

// ----- T-SECTION TURNS -----
void do_turn_left(){
  if(debug_mode) Serial.println("T-TURN LEFT");
  motor(-100, 100);
  delay(250);  // FIXED: Was 20ms - way too short! Need time to actually turn
  
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
  delay(250);  // FIXED: Was 20ms - way too short! Need time to actually turn
  
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
    delay(30);  // FIXED: Was 20ms - slightly longer for stability
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
    delay(30);  // FIXED: Was 20ms - slightly longer for stability
    reading();
  }
  motor(0, 0);
  delay(50);
}
