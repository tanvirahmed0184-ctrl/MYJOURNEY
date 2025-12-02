# Code Issues & Fixes

## 🔴 CRITICAL ISSUES

### Issue #1: Cross-Section Treated as Black Wall

**Problem:**
```cpp
if(sum == 6 && !just_junction){
  motor(0, 0);
  delay(150);
  reading();
  
  if(sum == 6){
    // STILL ALL BLACK = BLACK WALL / END
    Serial.println("BLACK WALL - END! STOPPING.");
    motor(0, 0);
    while(1); // Stop forever
  }
}
```

**What Happens:**
- Cross-section (4-way junction) has `sum == 6` initially
- After 150ms delay, still `sum == 6` (all sensors still on black)
- Code treats this as BLACK WALL → Robot stops forever ❌

**Fix Option 1: Time-Based Detection**
```cpp
if(sum == 6 && !just_junction){
  motor(0, 0);
  
  unsigned long start_time = millis();
  while(millis() - start_time < 500) {  // Check for 500ms
    reading();
    if(sum < 6) break;  // Line found, not a wall
  }
  
  reading();  // Final check
  if(sum == 6){
    // If still all black after 500ms = BLACK WALL
    Serial.println("BLACK WALL - END! STOPPING.");
    motor(0, 0);
    while(1);
  }
  else if(sum >= 3){
    // Cross-section or T-section
    // Choose direction based on strategy
    handle_cross_section();
  }
}
```

**Fix Option 2: Movement-Based Detection**
```cpp
if(sum == 6 && !just_junction){
  motor(0, 0);
  delay(150);
  reading();
  
  if(sum == 6){
    // Move forward slightly to check if it's a wall
    motor(80, 80);
    delay(100);
    reading();
    
    if(sum == 6){
      // Still all black = WALL
      Serial.println("BLACK WALL - END! STOPPING.");
      motor(0, 0);
      while(1);
    }
    else {
      // Cross-section - choose direction
      handle_cross_section();
    }
  }
}
```

**Fix Option 3: Direction Selection Logic**
```cpp
void handle_cross_section(){
  // Strategy: Always go forward, or alternate left/right
  // Example: Go forward
  motor(120, 120);
  delay(200);
  
  reading();
  if(sum == 0){
    // Forward path doesn't exist, try left
    do_turn_left();
  }
  // Otherwise continue forward
}
```

---

### Issue #2: Sharp Turn Edge Case

**Problem:**
```cpp
if((s[0] && sum <= 2 && !s[5]) || (s[5] && sum <= 2 && !s[0])){
```

**What if:** `s[0] == 1`, `s[5] == 1`, `sum == 2`?
- Both edges detected, but condition excludes this case
- Falls through to PID (may work, but not explicit)

**Fix:**
```cpp
if(sum <= 2){
  if(s[0] && !s[5]){
    // Sharp right turn
    bool turn_right = true;
    motor(lbase, rbase);
    delay(sharp_turn_forward_time);
    do_sharp_turn_right();
    return;
  }
  else if(s[5] && !s[0]){
    // Sharp left turn
    bool turn_right = false;
    motor(lbase, rbase);
    delay(sharp_turn_forward_time);
    do_sharp_turn_left();
    return;
  }
  else if(s[0] && s[5]){
    // Both edges detected - very sharp turn or junction
    // Choose based on last_turn memory
    if(last_turn == 'r'){
      motor(lbase, rbase);
      delay(sharp_turn_forward_time);
      do_sharp_turn_right();
    }
    else {
      motor(lbase, rbase);
      delay(sharp_turn_forward_time);
      do_sharp_turn_left();
    }
    return;
  }
}
```

---

### Issue #3: Motor Ramping Not Used in Lost Line

**Problem:**
```cpp
else {
  // LOST LINE - use remembered direction
  if (last_turn == 'l') {
    motor(-120, 120);   // Fixed speed, no ramping
  } else if (last_turn == 'r') {
    motor(120, -120);   // Fixed speed, no ramping
  }
}
```

**Fix:**
```cpp
else {
  // LOST LINE - use remembered direction
  Serial.print("LOST LINE - searching ");
  if (last_turn == 'l') {
    Serial.println("LEFT");
    // Use ramped values
    int target_l = -120;
    int target_r = 120;
    
    if(lmotor_actual > target_l) {
      lmotor_actual -= min(rate, lmotor_actual - target_l);
    } else {
      lmotor_actual += min(rate, target_l - lmotor_actual);
    }
    
    if(rmotor_actual < target_r) {
      rmotor_actual += min(rate, target_r - rmotor_actual);
    } else {
      rmotor_actual -= min(rate, rmotor_actual - target_r);
    }
    
    motor(lmotor_actual, rmotor_actual);
  } else if (last_turn == 'r') {
    Serial.println("RIGHT");
    // Similar ramping for right
    // ...
  }
}
```

---

## ⚠️ MINOR ISSUES

### Issue #4: PID Derivative Calculation

**Current:**
```cpp
float derivative = error[0] - error[1];
```

**Note:** This is correct, but `error[1]` needs to be initialized properly.

**Fix:**
```cpp
// In setup() or first iteration
error[1] = 0.0;  // Initialize previous error
```

---

### Issue #5: T-Section Alternation Logic

**Current:**
```cpp
if(last_T_turn == 'l'){ 
  do_turn_left(); 
  last_T_turn = 'r'; 
} else { 
  do_turn_right(); 
  last_T_turn = 'l'; 
}
```

**Note:** This alternates correctly, but consider:
- What if robot needs to go straight at T-section?
- What if specific direction is required?

**Enhancement:**
```cpp
// Add forward option
if(sum >= 3 && sum < 6){
  // Check if forward path exists
  motor(120, 120);
  delay(100);
  reading();
  
  if(sum >= 2){
    // Forward path exists
    just_junction = true;
    return;  // Continue forward
  }
  else {
    // No forward path, choose left/right
    if(last_T_turn == 'l'){ 
      do_turn_left(); 
      last_T_turn = 'r'; 
    } else { 
      do_turn_right(); 
      last_T_turn = 'l'; 
    }
  }
}
```

---

## 🟢 SUGGESTED IMPROVEMENTS

### Improvement #1: Add Speed Variation

**Current:** Fixed base speed (120)

**Enhancement:**
```cpp
// Adjust speed based on line position
int speed_factor = 1.0;
if(abs(avg) > 1.5){
  speed_factor = 0.7;  // Slow down on sharp curves
}

int lbase_actual = lbase * speed_factor;
int rbase_actual = rbase * speed_factor;
```

---

### Improvement #2: Add Encoder Feedback

**Enhancement:**
```cpp
// Add encoder pins
#define left_encoder 10
#define right_encoder 11

volatile int left_count = 0;
volatile int right_count = 0;

void left_encoder_isr(){
  left_count++;
}

void right_encoder_isr(){
  right_count++;
}

// Use for distance measurement at junctions
```

---

### Improvement #3: Add Debug Mode

**Enhancement:**
```cpp
#define DEBUG_MODE true

void debug_print(){
  if(DEBUG_MODE){
    Serial.print("S: ");
    for(int i=0; i<6; i++){ 
      Serial.print(s[i]); 
      Serial.print(" "); 
    }
    Serial.print("| sum="); Serial.print(sum);
    Serial.print(" | PID="); Serial.print(PID);
    Serial.print(" | L="); Serial.print(lmotor_actual);
    Serial.print(" R="); Serial.println(rmotor_actual);
  }
}
```

---

## 📋 TESTING CHECKLIST

### Before Competition:

- [ ] Test straight path following
- [ ] Test gentle curves (left/right)
- [ ] Test sharp 90° turns (left/right)
- [ ] Test T-sections (all directions)
- [ ] Test cross-sections (if applicable)
- [ ] Test lost line recovery
- [ ] Test black wall detection
- [ ] Test zigzag paths
- [ ] Test S-curves
- [ ] Test high-speed sections
- [ ] Test low-speed sections
- [ ] Calibrate PID gains (kp, kd)
- [ ] Calibrate threshold value
- [ ] Test motor ramping smoothness
- [ ] Test turn memory persistence

---

## 🎯 COMPETITION STRATEGY

### Recommended Settings:

```cpp
// For fast, smooth following:
int lbase = 140;   // Increase base speed
int rbase = 140;
int kp = 60;       // Increase proportional gain
int kd = 140;      // Increase derivative gain

// For precise, slow following:
int lbase = 100;
int rbase = 100;
int kp = 40;
int kd = 100;

// For aggressive turns:
int sharp_turn_speed = 120;  // Increase pivot speed
int sharp_turn_forward_time = 200;  // More forward before turn
```

---

## 🔧 QUICK FIX SUMMARY

**Priority 1 (Must Fix):**
1. ✅ Fix cross-section detection (use time-based or movement-based)
2. ✅ Handle sharp turn edge case (both edges detected)

**Priority 2 (Should Fix):**
3. ✅ Add motor ramping to lost line recovery
4. ✅ Initialize error[1] properly

**Priority 3 (Nice to Have):**
5. ✅ Add forward option at T-sections
6. ✅ Add speed variation based on curvature
7. ✅ Add encoder feedback
