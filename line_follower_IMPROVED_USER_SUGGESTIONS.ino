// ----- IMPROVED VERSION WITH USER'S EXCELLENT SUGGESTIONS -----
// Changes:
// 1. Better sharp turn detection (sum <= 2 instead of only one sensor)
// 2. Lost-line recovery using turn memory
// 3. Simplified junction logic (no U-turn, just STOP at black wall)

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

// TURN STATE & MEMORY - IMPROVED!
char turn = 's';         // 's'=straight, 'l'=left, 'r'=right (resets each loop)
char last_T_turn = 'l';  // alternate T-section turns
bool just_junction = false;  // ignore immediate re-detection

// MOTOR RAMP
int lmotor_actual = 0, rmotor_actual = 0;
int rate = 12;

// PID GAINS
int kp = 40;
int kd = 100;

// TURN PARAMETERS
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

// ----- SEMI-PID FUNCTION (IMPROVED) -----
void semi_pid(){
  reading();
  
  // RESET TURN MEMORY each loop (as user suggested)
  turn = 's';  // Default to straight
  
  // --- UPDATE TURN MEMORY BASED ON EDGE SENSORS ---
  // User's suggestion: track turns for lost-line recovery
  if(s[0] && !s[5]) turn = 'r';  // Right edge active
  if(s[5] && !s[0]) turn = 'l';  // Left edge active

  // --- JUNCTION DETECTION (ALL SENSORS) ---
  if(sum == 6 && !just_junction){
    motor(0, 0);  // Stop
    delay(150);   // Wait
    reading();    // Read again
    
    if(sum == 6){
      // STILL ALL BLACK after delay = BLACK WALL / END
      // User's suggestion: Just STOP, don't U-turn!
      Serial.println("BLACK WALL - END! STOPPING.");
      motor(0, 0);
      while(1); // Stop forever (or could do other action)
    }
    else if(sum >= 3){
      // T-SECTION or CROSS (some sensors still active)
      // Alternate left/right turns
      if(last_T_turn == 'l'){ 
        do_turn_left(); 
        last_T_turn = 'r'; 
      } else { 
        do_turn_right(); 
        last_T_turn = 'l'; 
      }
      just_junction = true;
      return;
    }
    // If sum < 3, just continue (passed junction line)
  }

  // --- IMPROVED SHARP TURN DETECTION ---
  // User's suggestion: sum <= 2 (allows edge + one more sensor)
  if((s[0] && sum <= 2 && !s[5]) || (s[5] && sum <= 2 && !s[0])){
    // Determine which direction
    bool turn_right = s[0];
    
    // Move forward before turn
    motor(lbase, rbase);
    delay(sharp_turn_forward_time);
    
    // Execute sharp turn
    if(turn_right){
      Serial.println("SHARP RIGHT (sum<=2)");
      do_sharp_turn_right();
    } else {
      Serial.println("SHARP LEFT (sum<=2)");
      do_sharp_turn_left();
    }
    return;
  }

  // --- NORMAL PID LINE FOLLOWING ---
  if(sum > 0){
    avg = (float)sensor_pos / (float)sum;
    error[0] = avg;
    float derivative = error[0] - error[1];
    PID = kp * error[0] + kd * derivative;
    error[1] = error[0];

    PID = constrain(PID, -120, 120);

    int lmotor_target = constrain((int)(lbase + PID), 0, pwm_cap);
    int rmotor_target = constrain((int)(rbase - PID), 0, pwm_cap);

    // --- MOTOR RAMP (FIXED) ---
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
    
    // Reset junction flag when back on line
    if(just_junction && sum < 6) just_junction = false;
  } 
  else {
    // --- IMPROVED LOST LINE RECOVERY ---
    // User's suggestion: Use turn memory to search!
    Serial.print("LOST LINE - searching ");
    if(turn == 'l'){
      Serial.println("LEFT");
      motor(-100, 100);  // Spin left to find line
    } else if(turn == 'r'){
      Serial.println("RIGHT");
      motor(100, -100);  // Spin right to find line
    } else {
      Serial.println("STRAIGHT");
      motor(lmotor_actual, rmotor_actual);  // Continue last direction
    }
  }

  // --- DEBUG ---
  Serial.print("S: ");
  for(int i=0; i<6; i++){ Serial.print(s[i]); Serial.print(" "); }
  Serial.print("| sum="); Serial.print(sum);
  Serial.print(" | turn="); Serial.print(turn);
  Serial.print(" | avg="); Serial.print(avg);
  Serial.print(" | PID="); Serial.print(PID);
  Serial.print(" | L="); Serial.print(lmotor_actual);
  Serial.print(" R="); Serial.println(rmotor_actual);
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
