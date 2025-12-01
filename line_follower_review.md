# Line Follower Robot Code Review

## Critical Bugs Found

### 1. **MOTOR RAMP BUG** (CRITICAL - Robot won't ramp properly!)

**Location:** Lines in motor ramping section

**Problem:**
```cpp
lmotor_actual -= min(rate, lmotor_actual - lmotor_actual);  // Always subtracts 0!
rmotor_actual -= min(rate, rmotor_actual - rmotor_actual);  // Always subtracts 0!
```

**Should be:**
```cpp
lmotor_actual -= min(rate, lmotor_actual - lmotor_target);
rmotor_actual -= min(rate, rmotor_actual - rmotor_target);
```

**Impact:** Motor deceleration won't work - motors will stay at high speed instead of ramping down smoothly.

---

## Logic Issues

### 2. **Turn Variable Never Used**
You detect turns with `s[0]` and `s[5]` and set `turn = 'l'` or `turn = 'r'`, but you **never use this variable** to actually turn!

**Current behavior:** The robot sets the turn flag but doesn't act on it for sharp turns.

**What you described:** "if a sharp left or right turn comes, it goes a bit forward until it reaches its half body then gets a sharp right or left turn"

**What the code does:** Sets `turn` variable but PID continues normally - no sharp turn action!

---

### 3. **T-Section Detection Confusion**

The code checks `sum == 6` **twice**:

**First check (U-turn detection):**
```cpp
if(sum == 6){
  delay(150);
  reading();
  if(sum == 0) { // dead-end
    do_uturn();
  }
}
```

**Second check (T-section detection):**
```cpp
if(sum == 6 && !just_uturn){
  delay(120);
  reading();
  if(sum != 0) { // T-section
    // alternate turns
  }
}
```

**Problems:**
- The second `sum == 6` check will never execute because `sum` was already checked and modified in the first block
- If you reach the second check, `sum` is no longer 6 (it was read again after the delay in first block)
- Logic flow is confusing and might not work as intended

---

### 4. **Missing Forward Movement for Sharp Turns**

You mentioned: "goes a bit forward until it reaches its half body then gets a sharp right or left turn"

**Missing in code:** No forward movement before executing sharp turns when `s[0]` or `s[5]` detect edges.

---

## What the Code Actually Does

✅ **Works correctly:**
- PID line following on straight line
- Sensor reading and thresholding
- Basic motor control with direction
- PWM capping

⚠️ **Partially works:**
- U-turn detection (checks all black then all white)
- T-section alternating turns (but buggy logic)
- Motor ramping (decrement broken)

❌ **Doesn't work as described:**
- Sharp turn handling (turn variable not used)
- No forward movement before sharp turns
- T-section and U-turn detection interfere with each other

---

## Suggestions for Improvement

### Fix #1: Correct Motor Ramp
### Fix #2: Implement Sharp Turn Logic
### Fix #3: Reorganize Junction Detection Logic
### Fix #4: Add State Machine for Better Control
### Fix #5: Calibration for Better Threshold
### Fix #6: Add Lost-Line Recovery

