// SIMPLE LINE FOLLOWER - CLEAN VERSION
// Just PID + Sharp Turns + Junction Detection
// No complicated line-lost spinning!

// ----- PIN SETUP -----
#define lmf 2
#define lmb 6
#define lms 3
#define rmf 4
#define rmb 5
#define rms 9

// ----- VARIABLES -----
int s[6], sum;
int position[6] = {3, 2, 1, -1, -2, -3};  // s[0]=rightmost, s[5]=leftmost
int threshold = 200;
int sensor_pos;

int lbase = 120;   // BASE SPEED
int rbase = 120;
int pwm_cap = 180;

// PID
float avg = 0.0;
float PID = 0.0;
float error = 0.0;
float last_error = 0.0;

// TURN MEMORY
char last_T_turn = 'l';  // alternate T-section turns
bool just_junction = false;

// PID GAINS
int kp = 40;
int kd = 100;

// TURN PARAMETERS
int sharp_turn_forward_time = 100;
int sharp_turn_speed = 100;

// DEBUG
bool debug_mode = true;

// ----- SETUP -----
void setup() {
  pinMode(lmf, OUTPUT);
  pinMode(lmb, OUTPUT);
  pinMode(rmf, OUTPUT);
  pinMode(rmb, OUTPUT);
  Serial.begin(9600);
  
  if(debug_mode){
    Serial.println("=== SIMPLE LINE FOLLOWER ===");
    Serial.print("Threshold: "); Serial.println(threshold);
    Serial.print("Base Speed: "); Serial.println(lbase);
    Serial.println("============================");
  }
}

// ----- MAIN LOOP -----
void loop() {
  semi_pid();
  delay(5);
}

// ----- MOTOR FUNCTION -----
void motor(int a, int b){
  a = constrain(a, -pwm_cap, pwm_cap);
  b = constrain(b, -pwm_cap, pwm_cap);

  // LEFT MOTOR
  if(a >= 0){
    digitalWrite(lmf, HIGH);
    digitalWrite(lmb, LOW);
  } else {
    a = -a;
    digitalWrite(lmf, LOW);
    digitalWrite(lmb, HIGH);
  }

  // RIGHT MOTOR
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

// ----- SENSOR READ -----
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

// ----- MAIN PID FUNCTION -----
void semi_pid(){
  reading();
  
  // === JUNCTION DETECTION (sum == 6) ===
  if(sum == 6 && !just_junction){
    if(debug_mode){
      Serial.println(">>> SUM=6 detected!");
      Serial.print("Sensors: ");
      for(int i=0; i<6; i++){ Serial.print(s[i]); }
      Serial.println();
    }
    
    motor(0, 0);          // STOP first
    delay(100);           // Brief pause
    
    motor(lbase, rbase);  // Go forward slowly
    delay(80);            // SHORTER: just 80ms to move slightly forward
    motor(0, 0);          // Stop again
    delay(50);            // Let it settle
    
    reading();            // Check again
    
    if(debug_mode){
      Serial.print("After forward, sum=");
      Serial.print(sum);
      Serial.print(" Sensors: ");
      for(int i=0; i<6; i++){ Serial.print(s[i]); }
      Serial.println();
    }
    
    if(sum == 6){
      // STILL ALL BLACK = END/BLACK WALL
      if(debug_mode) Serial.println(">>> BLACK WALL - STOPPING FOREVER!");
      motor(0, 0);
      while(1);  // Stop forever
    }
    else if(s[2] || s[3]){
      // MIDDLE SENSORS SEE LINE = CROSS SECTION
      if(debug_mode) Serial.println(">>> CROSS SECTION - going straight!");
      motor(lbase, rbase);
      delay(90);  // Cross the junction
      motor(0, 0);
      delay(50);
    }
    else if(sum == 0){
      // ALL WHITE = T-SECTION
      if(debug_mode) Serial.println(">>> T-SECTION - turning!");
      if(last_T_turn == 'l'){
        do_turn_right();
        last_T_turn = 'r';
      } else {
        do_turn_left();
        last_T_turn = 'l';
      }
    }
    else {
      // Some other pattern - maybe angled approach
      if(debug_mode) Serial.println(">>> Unexpected pattern after sum=6, treating as cross");
      motor(lbase, rbase);
      delay(100);
    }
    
    just_junction = true;
    return;
  }
  
  // === SHARP 90-DEGREE TURNS ===
  if((s[0] && sum <= 2 && !s[5]) || (s[5] && sum <= 2 && !s[0])){
    bool turn_right = s[0];
    
    if(debug_mode) Serial.println(turn_right ? ">>> SHARP RIGHT" : ">>> SHARP LEFT");
    
    motor(lbase, rbase);
    delay(sharp_turn_forward_time);
    
    if(turn_right){
      do_sharp_turn_right();
    } else {
      do_sharp_turn_left();
    }
    return;
  }
  
  // === NORMAL PID LINE FOLLOWING ===
  if(sum > 0){
    // LINE DETECTED - calculate position
    avg = (float)sensor_pos / (float)sum;
    error = avg;
    
    // PID calculation
    PID = kp * error + kd * (error - last_error);
    last_error = error;
    
    PID = constrain(PID, -150, 150);
    
    // Calculate motor speeds
    int lmotor = constrain((int)(lbase + PID), -pwm_cap, pwm_cap);
    int rmotor = constrain((int)(rbase - PID), -pwm_cap, pwm_cap);
    
    motor(lmotor, rmotor);
    
    // Reset junction flag when line is normal again
    if(just_junction && sum < 6) just_junction = false;
    
    if(debug_mode){
      Serial.print("S: ");
      for(int i=0; i<6; i++){ Serial.print(s[i]); }
      Serial.print(" | avg="); Serial.print(avg);
      Serial.print(" | PID="); Serial.print(PID);
      Serial.print(" | L="); Serial.print(lmotor);
      Serial.print(" R="); Serial.println(rmotor);
    }
  }
  else {
    // NO LINE (sum==0) - use last error to maintain direction
    // THIS IS THE KEY FIX - just like your working simple code!
    error = last_error;  // Keep last direction
    
    PID = kp * error + kd * (error - last_error);
    
    PID = constrain(PID, -150, 150);
    
    int lmotor = constrain((int)(lbase + PID), -pwm_cap, pwm_cap);
    int rmotor = constrain((int)(rbase - PID), -pwm_cap, pwm_cap);
    
    motor(lmotor, rmotor);
    
    if(debug_mode) Serial.println("NO LINE - maintaining last direction");
  }
}

// ----- T-SECTION TURNS -----
void do_turn_left(){
  if(debug_mode) Serial.println(">>> Turning LEFT");
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
  if(debug_mode) Serial.println(">>> Turning RIGHT");
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
