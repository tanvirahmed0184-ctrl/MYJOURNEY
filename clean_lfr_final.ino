// CLEAN LINE FOLLOWER - PATTERN BASED
// Simple, reliable detection logic
// Date: Dec 2, 2025

// ----- PINS -----
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

int lbase = 110;   // Base speed
int rbase = 110;
int pwm_cap = 180;

// PID
float avg = 0.0;
float PID = 0.0;
float error = 0.0;
float last_error = 0.0;

// GAINS (smooth)
int kp = 30;
int kd = 80;

// MEMORY
char last_T_turn = 'l';  // Alternate T-section turns
bool just_junction = false;

// DEBUG
bool debug_mode = true;

void setup() {
  pinMode(lmf, OUTPUT);
  pinMode(lmb, OUTPUT);
  pinMode(rmf, OUTPUT);
  pinMode(rmb, OUTPUT);
  Serial.begin(9600);
  delay(500);
  Serial.println("=== CLEAN LFR - PATTERN BASED ===");
  Serial.println("Starting in 2 seconds...");
  delay(2000);
}

void loop() {
  line_follow();
  delay(5);
}

// ----- MOTOR CONTROL -----
void motor(int a, int b){
  a = constrain(a, -pwm_cap, pwm_cap);
  b = constrain(b, -pwm_cap, pwm_cap);

  if(a >= 0){
    digitalWrite(lmf, HIGH);
    digitalWrite(lmb, LOW);
  } else {
    a = -a;
    digitalWrite(lmf, LOW);
    digitalWrite(lmb, HIGH);
  }

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

// ----- SENSOR READING -----
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

// ----- MAIN LINE FOLLOWING -----
void line_follow(){
  reading();
  
  // Debug output
  if(debug_mode){
    Serial.print("S:");
    for(int i=0; i<6; i++){ Serial.print(s[i]); }
    Serial.print(" sum="); Serial.print(sum);
  }
  
  // ============================================
  // PRIORITY 1: ALL BLACK (sum == 6)
  // ============================================
  if(sum == 6 && !just_junction){
    Serial.println(" -> ALL BLACK!");
    
    motor(0, 0);
    delay(50);
    
    // Peek forward to pass thin line (2.5cm)
    motor(lbase, rbase);
    delay(10);            // VERY SHORT - just to pass thin line
    motor(0, 0);
    delay(40);
    
    reading();
    Serial.print("After peek: ");
    for(int i=0; i<6; i++){ Serial.print(s[i]); }
    Serial.print(" sum="); Serial.println(sum);
    
    if(sum == 6){
      // Still all black = BLACK WALL / END
      Serial.println(">>> BLACK WALL - STOP!");
      motor(0, 0);
      while(1);
    }
    else if(s[2] || s[3]){
      // Middle sensors see line = CROSS SECTION
      Serial.println(">>> CROSS - going straight");
      motor(lbase, rbase);
      delay(30);            // Just enough to clear thin line
    }
    else {
      // No line ahead = T-SECTION
      Serial.println(">>> T-SECTION");
      if(last_T_turn == 'l'){
        turn_right();
        last_T_turn = 'r';
      } else {
        turn_left();
        last_T_turn = 'l';
      }
    }
    
    just_junction = true;
    return;
  }
  
  // ============================================
  // PRIORITY 2: SHARP RIGHT (edge + middle)
  // ============================================
  if(s[0] && (s[2] || s[3]) && !s[5]){
    Serial.println(" -> SHARP RIGHT!");
    motor(lbase, rbase);
    delay(40);  // Short forward movement
    sharp_turn_right();
    return;
  }
  
  // ============================================
  // PRIORITY 3: SHARP LEFT (edge + middle)
  // ============================================
  if(s[5] && (s[2] || s[3]) && !s[0]){
    Serial.println(" -> SHARP LEFT!");
    motor(lbase, rbase);
    delay(40);  // Short forward movement
    sharp_turn_left();
    return;
  }
  
  // ============================================
  // PRIORITY 4: NORMAL PID (line visible)
  // ============================================
  if(sum > 0){
    avg = (float)sensor_pos / (float)sum;
    error = avg;
    
    PID = kp * error + kd * (error - last_error);
    last_error = error;
    PID = constrain(PID, -150, 150);
    
    int lmotor = constrain((int)(lbase + PID), -pwm_cap, pwm_cap);
    int rmotor = constrain((int)(rbase - PID), -pwm_cap, pwm_cap);
    
    motor(lmotor, rmotor);
    
    // Reset junction flag
    if(just_junction && sum < 6) just_junction = false;
    
    if(debug_mode){
      Serial.print(" avg="); Serial.print(avg, 2);
      Serial.print(" PID="); Serial.print(PID, 1);
      Serial.print(" L="); Serial.print(lmotor);
      Serial.print(" R="); Serial.println(rmotor);
    }
  }
  // ============================================
  // PRIORITY 5: NO LINE (maintain direction)
  // ============================================
  else {
    error = last_error;
    PID = kp * error;
    PID = constrain(PID, -150, 150);
    
    int lmotor = constrain((int)(lbase + PID), -pwm_cap, pwm_cap);
    int rmotor = constrain((int)(rbase - PID), -pwm_cap, pwm_cap);
    
    motor(lmotor, rmotor);
    
    if(debug_mode) Serial.println(" NO LINE");
  }
}

// ============================================
// TURN FUNCTIONS
// ============================================

void turn_left(){
  Serial.println(">>> T-TURN LEFT");
  
  // Start turning immediately
  motor(-100, 100);
  delay(80);
  
  // Search for line with middle sensors
  for(int i=0; i<30; i++){
    reading();
    if(s[2] || s[3]){
      Serial.println("Found line!");
      motor(0, 0);
      delay(80);
      return;
    }
    motor(-90, 90);
    delay(15);
  }
  
  motor(0, 0);
  delay(80);
}

void turn_right(){
  Serial.println(">>> T-TURN RIGHT");
  
  // Start turning immediately
  motor(100, -100);
  delay(80);
  
  // Search for line with middle sensors
  for(int i=0; i<30; i++){
    reading();
    if(s[2] || s[3]){
      Serial.println("Found line!");
      motor(0, 0);
      delay(80);
      return;
    }
    motor(90, -90);
    delay(15);
  }
  
  motor(0, 0);
  delay(80);
}

void sharp_turn_left(){
  Serial.println(">>> SHARP LEFT");
  
  motor(-110, 110);
  
  // Turn until we see good line (sum >= 2)
  for(int i=0; i<35; i++){
    reading();
    if(sum >= 2 && (s[2] || s[3])){
      Serial.println("Found line!");
      motor(0, 0);
      delay(60);
      return;
    }
    delay(20);
  }
  
  motor(0, 0);
  delay(60);
}

void sharp_turn_right(){
  Serial.println(">>> SHARP RIGHT");
  
  motor(110, -110);
  
  // Turn until we see good line (sum >= 2)
  for(int i=0; i<35; i++){
    reading();
    if(sum >= 2 && (s[2] || s[3])){
      Serial.println("Found line!");
      motor(0, 0);
      delay(60);
      return;
    }
    delay(20);
  }
  
  motor(0, 0);
  delay(60);
}
