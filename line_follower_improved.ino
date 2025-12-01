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

// TURN & U-TURN STATE
char turn = 's';  // 's'=straight, 'l'=left, 'r'=right
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

// SHARP TURN DETECTION
int forward_delay = 100;  // delay to move forward before sharp turn (adjust for half body)

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
    // TCRT: HIGH on black line, LOW on white (adjust threshold as needed)
    // If your sensors are inverted, change to: s[i] = (s[i] > threshold) ? 0 : 1;
    s[i] = (s[i] > threshold) ? 1 : 0;
    sensor_pos += s[i] * position[i];
    sum += s[i];
  }
}

// ----- SEMI-PID FUNCTION -----
void semi_pid(){
  reading();

  // --- TURN DETECTION USING s0/s5 ---
  // s[0] = rightmost, s[5] = leftmost
  if(s[0] && !s[5]) turn = 'r';  // Right turn detected
  else if(s[5] && !s[0]) turn = 'l';  // Left turn detected
  else turn = 's';  // Straight

  // --- DEAD-END / U-TURN DETECTION (Check FIRST) ---
  if(sum == 6){
    motor(0, 0);
    delay(150);
    reading();
    if(sum == 0){ // All white = dead-end detected
      do_uturn();
      just_uturn = true;
      return;
    }
  }

  // --- SHARP TURN DETECTION (Only outer sensors see line) ---
  // If only s[0] or s[5] sees line with few sensors active = sharp turn
  if((s[0] && sum <= 2 && !s[5]) || (s[5] && sum <= 2 && !s[0])){
    // Move forward until half body passes (adjust delay based on robot size)
    motor(lbase, rbase);
    delay(forward_delay);
    motor(0, 0);
    delay(50);
    reading();
    
    // Now do sharp turn
    if(s[0] && !s[5]){
      do_turn_right();
    } else if(s[5] && !s[0]){
      do_turn_left();
    }
    return;
  }

  // --- T-SECTION / CROSS SECTION (Check BEFORE normal PID) ---
  if(sum == 6 && !just_uturn){
    // Move forward a bit to center robot on intersection
    motor(lbase, rbase);
    delay(50);
    motor(0, 0);
    delay(120);
    reading();
    if(sum != 0){ // Still on line = T-section detected
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

  // --- PID LINE FOLLOWING (Normal operation) ---
  if(sum > 0){
    avg = (float)sensor_pos / (float)sum;
    error[0] = avg;
    float derivative = error[0] - error[1];
    PID = kp * error[0] + kd * derivative;
    error[1] = error[0];

    // clamp PID
    PID = constrain(PID, -120, 120);

    // target motor speeds
    int lmotor_target = constrain((int)(lbase + PID), 0, pwm_cap);
    int rmotor_target = constrain((int)(rbase - PID), 0, pwm_cap);

    // --- MOTOR RAMP (FIXED BUG) ---
    if(lmotor_actual < lmotor_target){
      lmotor_actual += min(rate, lmotor_target - lmotor_actual);
    } else if(lmotor_actual > lmotor_target){
      lmotor_actual -= min(rate, lmotor_actual - lmotor_target);  // FIXED: was lmotor_actual - lmotor_actual
    }

    if(rmotor_actual < rmotor_target){
      rmotor_actual += min(rate, rmotor_target - rmotor_actual);
    } else if(rmotor_actual > rmotor_target){
      rmotor_actual -= min(rate, rmotor_actual - rmotor_target);  // FIXED: was rmotor_actual - rmotor_actual
    }

    motor(lmotor_actual, rmotor_actual);
  } else {
    // Lost line - use last known turn direction or stop
    motor(0, 0);
  }

  // reset just_uturn when robot moves away from intersection
  if(just_uturn && sum != 6) just_uturn = false;

  // --- DEBUG ---
  Serial.print("S: ");
  for(int i=0; i<6; i++){
    Serial.print(s[i]);
    Serial.print(" ");
  }
  Serial.print("| sum="); Serial.print(sum);
  Serial.print(" | avg="); Serial.print(avg);
  Serial.print(" | PID="); Serial.print(PID);
  Serial.print(" | turn="); Serial.print(turn);
  Serial.print(" | L="); Serial.print(lmotor_actual);
  Serial.print(" R="); Serial.println(rmotor_actual);
}

// ----- U-TURN / TURNS -----
void do_uturn(){
  motor(-120, 120);
  delay(500);
  motor(0, 0);
  delay(100);
}

void do_turn_left(){
  motor(-120, 120);
  delay(300);
  motor(0, 0);
  delay(100);
}

void do_turn_right(){
  motor(120, -120);
  delay(300);
  motor(0, 0);
  delay(100);
}
