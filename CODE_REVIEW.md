# Line Follower Robot Code Review

## Issues Found

### 1. **CRITICAL BUG: Motor Ramp Logic Error**
**Location:** Lines with motor ramp calculations
**Problem:** 
```cpp
lmotor_actual -= min(rate, lmotor_actual - lmotor_actual);  // WRONG!
```
Should be:
```cpp
lmotor_actual -= min(rate, lmotor_actual - lmotor_target);  // CORRECT
```
**Impact:** Motor ramp down doesn't work correctly, causing jerky movements.

### 2. **Sensor Logic Inverted**
**Location:** `reading()` function
**Problem:** 
```cpp
s[i] = (s[i] > threshold) ? 1 : 0;
```
If TCRT sensors read HIGH on black line, this is correct. But typically TCRT sensors read LOW on black line (reflective), so it might need to be inverted:
```cpp
s[i] = (s[i] > threshold) ? 0 : 1;  // If threshold detects white
```
**Note:** Verify your sensor behavior - if analogRead is HIGH on black line, current code is correct.

### 3. **Turn Variable Not Used**
**Location:** Turn detection sets `turn` but never uses it
**Problem:** The code detects turns with `s[0]` and `s[5]` but the `turn` variable is never used for sharp turn handling as you described.

### 4. **Missing Sharp Turn Handling**
**Problem:** You mentioned "if sharp left/right turn comes, goes forward until half body then sharp turn" but this logic is NOT implemented in the code.

### 5. **Sensor Position Array May Be Reversed**
**Location:** `position[6] = {3, 2, 1, -1, -2, -3}`
**Problem:** If A0 is your RIGHTMOST sensor, then:
- s[0] = rightmost (should be +3)
- s[5] = leftmost (should be -3)
But your array has position[0]=3, position[5]=-3, which seems correct IF s[0] is rightmost.
**Verify:** Make sure sensor positions match your physical layout.

### 6. **T-Section Detection Logic Issue**
**Problem:** The T-section detection happens AFTER motor ramp and motor update, which means:
- Robot continues moving during the 120ms delay
- May overshoot the intersection
- Should check BEFORE applying motor commands

### 7. **Dead-End Detection Timing**
**Problem:** Similar issue - dead-end check happens but robot may have moved during delay.

### 8. **No Forward Movement Before Sharp Turns**
**Problem:** As you described, robot should move forward until "half body" before sharp turns, but this is missing.

## What Your Code Actually Does

✅ **Works:**
- Basic PID line following
- Motor ramp up/down (though buggy)
- Dead-end detection with U-turn
- T-section detection with alternating turns
- Ignores T-sections after U-turn

❌ **Doesn't Work (as described):**
- Sharp turn detection and forward movement before turning
- The `turn` variable is set but never used
- Motor ramp down has a bug

## Suggestions

### 1. Fix Motor Ramp Bug
```cpp
// FIX THIS:
if(lmotor_actual > lmotor_target) 
  lmotor_actual -= min(rate, lmotor_actual - lmotor_target);  // was: lmotor_actual - lmotor_actual

if(rmotor_actual > rmotor_target) 
  rmotor_actual -= min(rate, rmotor_actual - rmotor_target);  // was: rmotor_actual - rmotor_actual
```

### 2. Add Sharp Turn Handling
Add logic to detect sharp turns and move forward before turning:
```cpp
// Detect sharp turn (only outer sensors see line)
if((s[0] && sum <= 2) || (s[5] && sum <= 2)){
  // Move forward a bit (half body length)
  motor(lbase, rbase);
  delay(100);  // Adjust based on robot speed and body length
  // Then do sharp turn
  if(s[0] && !s[5]) do_turn_right();
  else if(s[5] && !s[0]) do_turn_left();
}
```

### 3. Reorder Detection Logic
Check for intersections BEFORE applying motor commands:
```cpp
void semi_pid(){
  reading();
  
  // Check dead-end FIRST
  if(sum == 6){
    // ... dead-end logic
  }
  
  // Check T-section SECOND
  if(sum == 6 && !just_uturn){
    // ... T-section logic
    return;  // Exit early after turn
  }
  
  // Then do normal PID following
  // ... rest of code
}
```

### 4. Use the `turn` Variable
Actually use the turn detection for sharp turns:
```cpp
// After reading sensors
if(s[0] && !s[5]) turn = 'r';
else if(s[5] && !s[0]) turn = 'l';
else turn = 's';

// Use it for sharp turn detection
if(turn != 's' && sum <= 2){
  // Sharp turn detected, move forward then turn
}
```

### 5. Improve T-Section Detection
Add a small forward movement before turning at T-sections to center the robot:
```cpp
if(sum == 6 && !just_uturn){
  motor(lbase, rbase);  // Move forward a bit
  delay(50);
  motor(0,0);
  delay(120);
  reading();
  if(sum != 0){
    // Do turn
  }
}
```

### 6. Add Edge Case Handling
Handle case where robot loses line completely:
```cpp
if(sum == 0 && !just_uturn){
  // Lost line - use last known direction or search
  // Could use the 'turn' variable here
}
```
