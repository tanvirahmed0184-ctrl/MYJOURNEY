// ULTRA SIMPLE LINE FOLLOWER
// Just the basics - no fancy stuff

// ----- PINS -----
#define lmf 2
#define lmb 6
#define lms 3
#define rmf 4
#define rmb 5
#define rms 9

// ----- VARIABLES -----
int s[6], sum;
int position[6] = {3, 2, 1, -1, -2, -3};
int threshold = 200;
int sensor_pos;

int lbase = 110;   // Slightly slower for better detection
int rbase = 110;
int pwm_cap = 180;

// PID
float avg = 0.0;
float PID = 0.0;
float error = 0.0;
float last_error = 0.0;

// MEMORY
char last_T_turn = 'l';
bool just_junction = false;

// PID GAINS
int kp = 30;    // Lower for smoother
int kd = 80;

// DEBUG
bool debug_mode = true;

void setup() {
  pinMode(lmf, OUTPUT);
  pinMode(lmb, OUTPUT);
  pinMode(rmf, OUTPUT);
  pinMode(rmb, OUTPUT);
  Serial.begin(9600);
  delay(500);
  Serial.println("=== ULTRA SIMPLE LFR ===");
  Serial.println("Starting in 2 seconds...");
  delay(2000);
}

void loop() {
  semi_pid();
  delay(5);
}

// ----- MOTOR -----
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

// ----- READING -----
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

// ----- MAIN LOGIC -----
void semi_pid(){
  reading();
  
  // DEBUG: Print what we see
  if(debug_mode){
    Serial.print("S:");
    for(int i=0; i<6; i++){ Serial.print(s[i]); }
    Serial.print(" sum="); Serial.print(sum);
  }
  
  // === CASE 1: ALL BLACK (sum == 6) ===
  if(sum == 6 && !just_junction){
    Serial.println(" -> ALL BLACK!");
    motor(0, 0);
    delay(80);
    
    // Peek forward slightly
    motor(lbase, rbase);
    delay(15);
    motor(0, 0);
    delay(60);
    
    reading();
    Serial.print("After peek, sum="); Serial.println(sum);
    
    if(sum == 6){
      Serial.println("BLACK WALL - STOP!");
      motor(0, 0);
      while(1);
    }
    else if(s[2] || s[3]){
      Serial.println("CROSS - go straight");
      motor(lbase, rbase);
      delay(50);
    }
    else {
      Serial.println("T-SECTION - turning");
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
  
  // === CASE 2: SHARP TURN (edge sensor only) ===
  // More relaxed: sum <= 3 instead of <= 2
  if((s[0] && sum <= 3 && !s[5])){
    Serial.println(" -> SHARP RIGHT!");
    motor(lbase, rbase);
    delay(80);
    sharp_turn_right();
    return;
  }
  
  if((s[5] && sum <= 3 && !s[0])){
    Serial.println(" -> SHARP LEFT!");
    motor(lbase, rbase);
    delay(80);
    sharp_turn_left();
    return;
  }
  
  // === CASE 3: NORMAL LINE FOLLOWING ===
  if(sum > 0){
    avg = (float)sensor_pos / (float)sum;
    error = avg;
    
    PID = kp * error + kd * (error - last_error);
    last_error = error;
    PID = constrain(PID, -150, 150);
    
    int lmotor = constrain((int)(lbase + PID), -pwm_cap, pwm_cap);
    int rmotor = constrain((int)(rbase - PID), -pwm_cap, pwm_cap);
    
    motor(lmotor, rmotor);
    
    if(just_junction && sum < 6) just_junction = false;
    
    if(debug_mode){
      Serial.print(" avg="); Serial.print(avg);
      Serial.print(" PID="); Serial.print(PID);
      Serial.print(" L="); Serial.print(lmotor);
      Serial.print(" R="); Serial.println(rmotor);
    }
  }
  else {
    // === CASE 4: NO LINE ===
    error = last_error;
    PID = kp * error;
    PID = constrain(PID, -150, 150);
    
    int lmotor = constrain((int)(lbase + PID), -pwm_cap, pwm_cap);
    int rmotor = constrain((int)(rbase - PID), -pwm_cap, pwm_cap);
    
    motor(lmotor, rmotor);
    
    if(debug_mode) Serial.println(" NO LINE - last direction");
  }
}

// ----- SIMPLE TURNS -----
void turn_left(){
  Serial.println(">>> LEFT TURN");
  motor(-90, 90);
  delay(150);  // Blind turn time
  
  // Search for line
  for(int i=0; i<30; i++){
    reading();
    if(s[2] || s[3]){
      Serial.println("Found line!");
      motor(0, 0);
      delay(100);
      return;
    }
    motor(-90, 90);
    delay(20);
  }
  motor(0, 0);
  delay(100);
}

void turn_right(){
  Serial.println(">>> RIGHT TURN");
  motor(90, -90);
  delay(150);  // Blind turn time
  
  // Search for line
  for(int i=0; i<30; i++){
    reading();
    if(s[2] || s[3]){
      Serial.println("Found line!");
      motor(0, 0);
      delay(100);
      return;
    }
    motor(90, -90);
    delay(20);
  }
  motor(0, 0);
  delay(100);
}

void sharp_turn_left(){
  Serial.println(">>> SHARP LEFT");
  motor(-100, 100);
  
  // Keep turning until we see 2+ sensors
  for(int i=0; i<40; i++){
    reading();
    if(sum >= 2){
      Serial.println("Found line!");
      motor(0, 0);
      delay(80);
      return;
    }
    delay(25);
  }
  motor(0, 0);
  delay(80);
}

void sharp_turn_right(){
  Serial.println(">>> SHARP RIGHT");
  motor(100, -100);
  
  // Keep turning until we see 2+ sensors
  for(int i=0; i<40; i++){
    reading();
    if(sum >= 2){
      Serial.println("Found line!");
      motor(0, 0);
      delay(80);
      return;
    }
    delay(25);
  }
  motor(0, 0);
  delay(80);
}
