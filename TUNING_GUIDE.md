# Line Follower Robot Tuning Guide

## Quick Start Checklist

Before uploading the fixed code:

1. ✅ Check all 6 sensors are connected (A0-A5)
2. ✅ Verify motor pins match your motor driver
3. ✅ Test motors spin in correct direction
4. ✅ Calibrate sensor threshold value
5. ✅ Adjust base speed for your robot weight/motors
6. ✅ Fine-tune PID gains
7. ✅ Adjust turn timing delays

---

## 1. Sensor Threshold Calibration

**Current value:** `threshold = 512;`

### How to calibrate:

**Method A: Serial Monitor**
```cpp
void setup(){
  Serial.begin(9600);
  while(1){
    for(int i=0; i<6; i++){
      Serial.print("S"); Serial.print(i); Serial.print("=");
      Serial.print(analogRead(i)); Serial.print(" ");
    }
    Serial.println();
    delay(200);
  }
}
```

1. Upload this test code
2. Place robot on **white surface** - note the values (e.g., 100-200)
3. Place robot on **black line** - note the values (e.g., 800-950)
4. Calculate threshold: `(white_max + black_min) / 2`
5. Example: white=200, black=800 → threshold = 500

**Method B: Auto-calibration**
```cpp
void calibrate(){
  int min_val = 1023, max_val = 0;
  
  Serial.println("Calibrating... move robot over white and black");
  delay(2000);
  
  for(int j=0; j<100; j++){
    for(int i=0; i<6; i++){
      int val = analogRead(i);
      if(val < min_val) min_val = val;
      if(val > max_val) max_val = val;
    }
    delay(20);
  }
  
  threshold = (min_val + max_val) / 2;
  Serial.print("Threshold set to: "); Serial.println(threshold);
}
```

**Typical values:**
- White paper: 50-250
- Black tape: 700-950
- Good threshold: 400-600

---

## 2. Base Speed Tuning

**Current values:**
```cpp
int lbase = 120;   // Left motor base speed
int rbase = 120;   // Right motor base speed
```

### Symptoms and fixes:

| Problem | Likely Cause | Fix |
|---------|--------------|-----|
| Robot too slow | Base speed too low | Increase lbase/rbase to 140-160 |
| Robot wobbles/oscillates | Too fast for PID to correct | Decrease lbase/rbase to 80-100 |
| Robot goes straight off line | Base speed too high | Decrease to 100-120 |
| One side drifts | Motor imbalance | Adjust lbase or rbase (e.g., lbase=120, rbase=125) |

**Testing:**
1. Start with 100
2. Increase by 10 until robot follows line smoothly
3. If it overshoots curves, reduce by 10

---

## 3. PID Gain Tuning

**Current values:**
```cpp
int kp = 40;   // Proportional gain
int kd = 100;  // Derivative gain
```

### Tuning Process:

**Step 1: Set KD=0, tune KP only**
1. Set `kd = 0;`
2. Start with `kp = 10;`
3. Increase KP until robot oscillates (wobbles left-right)
4. Reduce KP to 50-70% of oscillation point

**Step 2: Tune KD**
1. Start with `kd = 0;`
2. Increase by 10 until oscillations reduce
3. Too high KD = sluggish response
4. Optimal KD = smooth corrections without overshoot

### Common PID symptoms:

| Behavior | Problem | Solution |
|----------|---------|----------|
| Smooth but slow corrections | KP too low | Increase kp by 10-20 |
| Fast oscillations (wobble) | KP too high | Decrease kp by 10-20 |
| Overshoots and bounces back | KD too low | Increase kd by 20-50 |
| Sluggish on curves | KD too high | Decrease kd by 20-50 |
| Perfect on straight, bad on curves | Need more KD | Increase kd |
| Perfect on curves, wobbles on straight | Too much KD | Decrease kd |

**Recommended ranges:**
- KP: 20-80 (start with 40)
- KD: 50-200 (start with 100)

---

## 4. Turn Timing Adjustments

### Sharp Turn Forward Time
```cpp
int sharp_turn_forward_time = 150; // ms
```

**What it does:** How long robot moves forward before executing sharp turn

**Tuning:**
- Too short: Robot turns before fully reaching turn → loses line
- Too long: Robot overshoots past turn → loses line
- Ideal: Robot's center is over the turning point

**Typical values:** 100-200ms depending on base speed and robot length

**Calculate:** `time = (robot_length / 2) / speed`

---

### U-Turn Timing
```cpp
void do_uturn(){
  motor(-120, 120);
  delay(600);  // ← Adjust this
}
```

**Tuning:**
- Measure how long your robot takes to spin 180°
- Test on a smooth surface
- Adjust delay until it consistently faces backward

**Too short:** Doesn't complete 180°, goes back into dead-end
**Too long:** Overshoots, wastes time

---

### T-Section Turn Timing
```cpp
void do_turn_left(){
  motor(-100, 100);
  delay(350);  // ← Adjust this
}
```

**Tuning:**
- Should turn ~90° to face new line
- Code also checks for middle sensors to detect line
- Delay is backup in case sensor check fails

**Typical values:** 250-400ms for 90° turn

---

### Junction Confirmation Delay
```cpp
if(sum == 6){
  delay(150);  // ← Adjust this
  reading();
}
```

**What it does:** Waits to confirm if it's really a junction or just crossing a line

**Tuning:**
- Too short: Might mistake quick line crossing for junction
- Too long: Wastes time at every junction
- Optimal: Just enough to move past the junction trigger point

**Typical values:** 100-200ms

---

## 5. Motor Ramp Rate

```cpp
int rate = 12; // max PWM change per loop
```

**What it does:** How quickly motors accelerate/decelerate

**Tuning:**
- Higher value = faster speed changes (more aggressive)
- Lower value = smoother transitions (gentler)

**Symptoms:**
- rate=5: Very smooth but slow to respond to curves
- rate=20: Fast response but jerky movements
- rate=12: Good balance (recommended)

**When to adjust:**
- Robot jerks on speed changes → decrease rate (8-10)
- Robot too slow to respond to curves → increase rate (15-20)

---

## 6. PWM Cap

```cpp
int pwm_cap = 180; // MAX PWM (out of 255)
```

**What it does:** Maximum motor speed limit

**Tuning:**
- 180/255 = ~70% max speed (conservative, recommended)
- 255 = full power (may be too fast for good line tracking)
- 150 = ~60% (good for learning/testing)

**Adjust based on:**
- Track complexity (simple = higher, complex = lower)
- Motor power (weak motors = higher, strong = lower)
- Battery voltage (fresh battery = lower cap, weak = higher)

---

## 7. Advanced: Lost Line Recovery

Add timeout for lost line:

```cpp
unsigned long lost_time = 0;
int lost_timeout = 500; // ms

if(sum == 0){
  if(lost_time == 0) lost_time = millis();
  
  if(millis() - lost_time > lost_timeout){
    // Search pattern
    motor(-100, 100); // spin to search
  }
} else {
  lost_time = 0; // reset
}
```

---

## Testing Procedure

### Test 1: Straight Line
1. Place robot on straight line
2. Watch for smooth following
3. Check serial monitor for sensor readings
4. Adjust PID if wobbling

### Test 2: Gentle Curves
1. Test on gradual curves
2. Should follow smoothly without slowing
3. If loses line, increase KP or base speed

### Test 3: Sharp 90° Turns
1. Mark turn with tape to see where robot starts turning
2. Should detect with edge sensor, move forward, then turn
3. Adjust `sharp_turn_forward_time` if needed

### Test 4: T-Sections
1. Test on T-junction
2. Should stop, confirm, then turn
3. Check if alternates left/right correctly
4. Adjust turn delays if doesn't complete 90°

### Test 5: Dead-End
1. Test U-turn at dead-end
2. Should detect all black → all white
3. Should complete full 180° turn
4. Adjust U-turn delay if needed

### Test 6: Complex Track
1. Combine all elements
2. Run full course multiple times
3. Note where it fails
4. Fine-tune specific problem areas

---

## Common Problems & Solutions

### Robot loses line on sharp turns
**Causes:**
- Base speed too high
- Sharp turn detection not triggering
- Forward time before turn incorrect

**Fixes:**
1. Decrease base speed by 10-20
2. Check edge sensors (s[0], s[5]) are working
3. Adjust `sharp_turn_forward_time`

---

### Robot oscillates (wobbles) on straight line
**Causes:**
- KP too high
- Base speed too high for PID to correct
- Sensors misaligned

**Fixes:**
1. Decrease kp by 10-20
2. Decrease base speed by 10-20
3. Check sensor array is perpendicular to line

---

### Robot doesn't detect T-sections
**Causes:**
- Threshold wrong (sensors don't all read black)
- Junction confirmation delay wrong
- Track T-section not wide enough

**Fixes:**
1. Recalibrate threshold
2. Check serial monitor at T-section - do all 6 sensors read 1?
3. Increase line width at junctions

---

### Robot spins at T-sections but doesn't find new line
**Causes:**
- Turn delay too short (doesn't turn 90°)
- Turn motors too weak
- Sensor check condition wrong

**Fixes:**
1. Increase turn delay by 50-100ms
2. Increase turn motor speed (currently -100/100)
3. Check middle sensors (s[2], s[3]) detect new line

---

### U-turn doesn't complete 180°
**Causes:**
- Delay too short
- Motor power insufficient
- Battery weak

**Fixes:**
1. Increase U-turn delay in `do_uturn()`
2. Increase motor speed (-120/120 → -150/150)
3. Charge battery

---

### Motors behave erratically
**Causes:**
- Motor pins incorrect
- Motor driver wiring wrong
- PWM pins wrong

**Fixes:**
1. Verify pin definitions match hardware
2. Test each motor individually
3. Check motor driver power supply

---

## Quick Reference Table

| Parameter | Default | Typical Range | What it affects |
|-----------|---------|---------------|-----------------|
| threshold | 512 | 400-600 | Black/white detection |
| lbase | 120 | 80-160 | Left motor base speed |
| rbase | 120 | 80-160 | Right motor base speed |
| kp | 40 | 20-80 | Correction strength |
| kd | 100 | 50-200 | Correction smoothness |
| rate | 12 | 8-20 | Acceleration rate |
| pwm_cap | 180 | 150-255 | Maximum motor speed |
| sharp_turn_forward_time | 150 | 100-250 | Forward before sharp turn |
| U-turn delay | 600 | 400-800 | 180° turn time |
| T-turn delay | 350 | 250-500 | 90° turn time |
| Junction confirm delay | 150 | 100-200 | Junction confirmation |

---

## Recommended Tuning Order

1. **Threshold** (most critical - nothing works if this is wrong)
2. **Base speed** (affects everything else)
3. **KP** (basic line following)
4. **KD** (smoothness)
5. **Sharp turn forward time** (sharp turn handling)
6. **Turn delays** (U-turn and T-section timing)
7. **Ramp rate** (fine-tuning smoothness)

---

Good luck with your line follower! 🤖
