# 🚀 Line Follower Robot - Quick Reference Guide

## 📋 KEY CHANGES IN FIXED VERSION

### 🔴 CRITICAL FIX: Cross Junction Detection

**Problem in Original Code:**
```cpp
// Treated BOTH T-sections AND cross junctions as T-sections
if(sum >= 3){
  // Always alternates turn (WRONG for cross!)
  do_alternate_turn();
}
```

**Fixed Code:**
```cpp
if(sum >= 3){
  if(s[2] || s[3]){
    // Middle sensors ON = Forward path exists = CROSS
    motor(lbase, rbase);  // Go straight
    delay(250);
  }
  else {
    // Middle sensors OFF = No forward path = T-SECTION
    do_alternate_turn();  // Turn left/right
  }
}
```

**How to Test:**
- Cross junction: Robot should go straight through
- T-junction: Robot should turn left or right
- If robot turns at cross, fix not applied correctly!

---

## 🎯 SENSOR READING QUICK REFERENCE

### Sensor Position Map
```
Position:  [0]  [1]  [2]  [3]  [4]  [5]
Weight:    +3   +2   +1   -1   -2   -3
```

### Common Sensor Patterns

| Pattern | Meaning | Action |
|---------|---------|--------|
| `[0][0][1][1][0][0]` | Centered on line | Go straight |
| `[0][1][1][1][0][0]` | Line slightly right | Turn left |
| `[0][0][0][1][1][1]` | Line left of robot | Turn right |
| `[1][0][0][0][0][0]` | Sharp right turn | Pivot right |
| `[0][0][0][0][0][1]` | Sharp left turn | Pivot left |
| `[1][1][1][1][1][1]` | Junction or wall | Special logic |
| `[0][0][0][0][0][0]` | Lost line | Search using memory |

---

## ⚙️ TUNING PARAMETERS

### Motor Speeds
```cpp
int lbase = 120;   // Left motor base speed (0-255)
int rbase = 120;   // Right motor base speed
int pwm_cap = 180; // Maximum motor speed
```

**Tuning Tips:**
- Start with 100-120 for testing
- Increase for faster runs (competition)
- Decrease if robot wobbles or overshoots turns
- Keep lbase ≈ rbase (unless compensating for motor differences)

### PID Gains
```cpp
int kp = 50;   // Proportional gain
int kd = 120;  // Derivative gain
```

**Effect of Each Gain:**

| Parameter | Too Low | Good Value | Too High |
|-----------|---------|------------|----------|
| **kp** | Slow response, can't follow curves | Quick centering | Oscillation, zigzag motion |
| **kd** | Oscillates, overshoots | Smooth, damped | Sluggish, delayed response |

**Tuning Process:**
1. Set kd=0, adjust kp until robot follows straight line without oscillating
2. Add kd gradually until oscillations disappear
3. Test on curves - increase kp if robot leaves line
4. Test on sharp turns - may need to reduce speeds

### Sharp Turn Settings
```cpp
int sharp_turn_forward_time = 150;  // ms
int sharp_turn_speed = 100;         // PWM value
```

**Tuning Tips:**
- **forward_time:** Increase if robot doesn't clear corner edge
- **turn_speed:** Increase for faster pivots (but less control)

### Threshold (Sensor Calibration)
```cpp
int threshold = 512;  // 0-1023 (ADC range)
```

**How to Calibrate:**
1. Place robot on white surface
2. Read sensor values: `analogRead(0)` to `analogRead(5)`
3. Place robot on black line
4. Read sensor values again
5. Set threshold to midpoint:
   ```
   threshold = (white_value + black_value) / 2
   ```

**Example:**
- White surface: ~850
- Black line: ~150
- Threshold: (850 + 150) / 2 = 500 ✓

---

## 🔧 COMMON ISSUES & SOLUTIONS

### Issue 1: Robot Oscillates (Zigzags)
**Cause:** kp too high or kd too low

**Solution:**
1. Reduce kp by 10-20%
2. Increase kd by 20-30%
3. Reduce base speed if still oscillating

### Issue 2: Robot Leaves Line on Curves
**Cause:** Not responding fast enough

**Solution:**
1. Increase kp by 20%
2. Reduce base speed (gives more time to react)
3. Check sensor calibration

### Issue 3: Robot Overshoots Sharp Turns
**Cause:** Too much forward momentum

**Solution:**
1. Reduce `sharp_turn_forward_time` (try 100ms)
2. Reduce base speed
3. Increase `sharp_turn_speed` for faster pivots

### Issue 4: Lost Line Recovery Fails
**Cause:** Wrong direction or memory not updated

**Solution:**
1. Check that edge sensors (0 and 5) are working
2. Verify `last_turn` updates correctly
3. Increase pivot speed if searching too slowly

### Issue 5: Junction Detection Unreliable
**Cause:** Timing or sensor positioning

**Solution:**
1. Adjust 150ms delay (increase if robot moves too far)
2. Clean sensors
3. Verify all 6 sensors can read full-width line simultaneously
4. Check sensor spacing matches line width

### Issue 6: Motors Don't Move
**Cause:** Power, wiring, or PWM issues

**Solution:**
1. Check battery voltage (needs >6V for most motors)
2. Verify motor driver connections
3. Test motors individually: `motor(150, 150);` in setup()
4. Check PWM pins (3, 9) support analogWrite()

---

## 📊 EXPECTED PERFORMANCE METRICS

| Metric | Target Value |
|--------|--------------|
| Straight line following | ±2mm deviation |
| Gentle curve (R>20cm) | 100% success |
| Sharp turn (90°) | 300-400ms completion |
| Junction detection | <200ms response |
| Lost line recovery | <300ms to re-acquire |
| Black wall stop distance | <5cm |

---

## 🧪 PRE-COMPETITION CHECKLIST

### Hardware
- [ ] All 6 sensors clean and functional
- [ ] Motor driver connected correctly
- [ ] Battery fully charged (check voltage)
- [ ] Wheels secure and balanced
- [ ] Sensor height: 2-5mm above track
- [ ] Sensor spacing matches line width

### Software
- [ ] Cross junction fix applied
- [ ] Threshold calibrated for competition lighting
- [ ] PID gains tuned on practice track
- [ ] Base speed set (conservative for first run)
- [ ] Debug mode OFF for faster execution

### Testing
- [ ] Straight line: smooth tracking
- [ ] Gentle curves: follows without leaving
- [ ] Sharp turns (90°): completes reliably
- [ ] T-junctions: alternates correctly
- [ ] **Cross junctions: goes STRAIGHT** ← Critical!
- [ ] Black wall: stops immediately
- [ ] Lost line: recovers within 1 second
- [ ] Complex paths: handles back-to-back obstacles

---

## 🎮 COMPETITION STRATEGY

### Run 1: Conservative
- Set `lbase = rbase = 100` (slower but safer)
- Complete the track to understand layout
- Note problem areas

### Run 2: Optimized
- Increase speed: `lbase = rbase = 140`
- Adjust sharp_turn_forward_time if needed
- Focus on smooth execution

### Run 3: Maximum Speed (if allowed)
- Push limits: `lbase = rbase = 160-180`
- Only if previous runs were flawless
- Risk vs reward - faster isn't always better!

---

## 🐛 DEBUGGING TIPS

### Serial Monitor Output
```
S: 0 0 1 1 0 0 | sum=2 | turn=s | avg=0.0 | PID=0 | L=120 R=120
```

**What to look for:**
- **S values:** Should match what sensors see
- **sum:** Should be 1-3 on normal line, 6 at junctions
- **avg:** Should be near 0 when centered
- **PID:** Should be small (<50) on straight, larger on curves
- **L/R:** Should be similar on straight, diverge on turns

### Enable Debug Mode
```cpp
bool debug_mode = true;  // Testing
bool debug_mode = false; // Competition (faster)
```

### Quick Motor Test (in setup())
```cpp
void setup() {
  // ... pin setup ...
  
  // Test left motor
  motor(150, 0);
  delay(1000);
  
  // Test right motor
  motor(0, 150);
  delay(1000);
  
  // Stop
  motor(0, 0);
  delay(2000);
}
```

---

## 📱 COMPETITION DAY REMINDERS

1. **Arrive Early** - Time for sensor calibration
2. **Bring Tools** - Screwdriver, multimeter, spare wires
3. **Spare Parts** - Extra sensors, batteries
4. **Backup Code** - Save multiple versions on laptop
5. **Stay Calm** - If run fails, you usually get retries
6. **Document Settings** - Write down working PID values

---

## 🏆 SUCCESS CRITERIA

**Minimum (Bronze Level):**
- Follows straight lines
- Handles gentle curves
- Completes basic track

**Target (Silver Level):**
- All of above, plus:
- Navigates sharp turns
- Handles T-junctions
- Lost line recovery

**Excellence (Gold Level):**
- All of above, plus:
- Goes straight through cross junctions ← Fixed!
- Fast completion time
- Smooth, no oscillation
- Reliable black wall detection

---

## 🔗 USEFUL RESOURCES

### Sensor Calibration Formula
```cpp
normalized_value = (sensor_raw - threshold) > 0 ? 1 : 0;
```

### PID Calculation (for reference)
```cpp
avg = sensor_pos / sum;           // -3.0 to +3.0
error = avg;                      // Current position
derivative = error - last_error;  // Rate of change
PID = kp * error + kd * derivative;
left_motor = base + PID;
right_motor = base - PID;
```

### Lost Line Decision Tree
```
sum == 0?
├─ Yes → Check last_turn
│  ├─ 'l' → Pivot left
│  ├─ 'r' → Pivot right
│  └─ 's' → Continue straight
└─ No → Normal PID control
```

---

## 📧 NOTES

**Author:** AI Assistant (Claude)  
**Date:** December 2, 2025  
**Version:** Fixed (Cross Junction Support)  
**Files:**
- `line_follower_FIXED.ino` - Corrected code
- `LINE_FOLLOWER_CODE_REVIEW.md` - Detailed analysis
- `SIMULATION_VISUALIZATIONS.md` - Step-by-step scenarios
- `QUICK_REFERENCE_GUIDE.md` - This file

**Key Improvement:** Fixed cross junction detection by checking middle sensors (s[2], s[3]) to distinguish between cross (+) and T-sections (⊤).

---

**Good luck! You've got this! 🏁**
