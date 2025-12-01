# Line Follower Robot Testing Checklist

Print this out and check off each item as you test your robot! ✓

---

## Pre-Upload Checklist

### Hardware Verification
- [ ] All 6 sensors connected to A0-A5 (A0=rightmost, A5=leftmost)
- [ ] Left motor forward pin → Pin 2
- [ ] Left motor backward pin → Pin 6
- [ ] Left motor PWM (speed) → Pin 3
- [ ] Right motor forward pin → Pin 4
- [ ] Right motor backward pin → Pin 5
- [ ] Right motor PWM (speed) → Pin 9
- [ ] Motor driver power connected
- [ ] Arduino powered (battery or USB)
- [ ] All connections secure (no loose wires)

### Software Verification
- [ ] Arduino IDE installed
- [ ] Correct board selected (Tools → Board)
- [ ] Correct port selected (Tools → Port)
- [ ] `line_follower_FIXED.ino` opened
- [ ] Code compiles without errors (Verify button)

---

## Initial Calibration

### Step 1: Sensor Test
- [ ] Upload sensor test code (see TUNING_GUIDE.md)
- [ ] Open Serial Monitor (9600 baud)
- [ ] Place robot on **white surface**
- [ ] Record white values: S0=___ S1=___ S2=___ S3=___ S4=___ S5=___
- [ ] Place robot on **black line**
- [ ] Record black values: S0=___ S1=___ S2=___ S3=___ S4=___ S5=___
- [ ] Calculate threshold: (highest_white + lowest_black) / 2 = ___
- [ ] Update `threshold` value in main code
- [ ] Re-upload main code

### Step 2: Motor Direction Test
- [ ] Place robot on stand (wheels off ground)
- [ ] Upload main code
- [ ] Place finger on middle sensors (simulate line)
- [ ] Check left motor spins **forward**
- [ ] Check right motor spins **forward**
- [ ] If backward, swap motor forward/backward pins
- [ ] Re-test until both spin forward

---

## Basic Function Tests

### Test 1: Straight Line Following
**Track:** Simple straight line, 3-5 feet long

- [ ] Robot follows line smoothly
- [ ] No excessive wobbling (slight wobble OK)
- [ ] Maintains center of line
- [ ] Speed seems reasonable (not too slow/fast)
- [ ] No jerky movements

**If fails:**
- [ ] Check sensor threshold calibration
- [ ] Try decreasing base speed (lbase=100, rbase=100)
- [ ] Try decreasing KP (kp=30)
- [ ] Check sensors are aligned perpendicular to line

**Serial Monitor Check:**
```
Expected output:
S: 0 0 1 1 0 0 | avg=0.0 | PID=0.0 | L=120 R=120
(middle sensors active, avg near 0, motors similar speeds)
```

---

### Test 2: Gentle Curve
**Track:** Smooth 45° curve

- [ ] Robot follows curve smoothly
- [ ] Doesn't trigger sharp turn behavior
- [ ] PID adjusts speed appropriately
- [ ] Stays on line through entire curve

**If fails:**
- [ ] Increase KD if overshooting (kd=120)
- [ ] Increase base speed if too slow (lbase=130)
- [ ] Check PID is responding (watch serial monitor)

**Serial Monitor Check:**
```
On right curve:
S: 1 1 1 0 0 0 | avg=2.0 | PID=positive | L>R
(left motor faster than right)
```

---

### Test 3: Sharp Right Turn (90°)
**Track:** 90° right angle turn

**Expected Behavior:**
1. Right edge sensor (s0) detects turn
2. Robot moves forward ~150ms
3. Robot spins right in place
4. Continues when line found

**Checklist:**
- [ ] Robot detects turn (doesn't go straight off)
- [ ] Robot moves forward before turning
- [ ] Robot spins right (not just curves)
- [ ] Robot finds line after turn
- [ ] Continues following after turn

**If fails:**
- [ ] Check s[0] sensor is working (serial monitor)
- [ ] Increase `sharp_turn_forward_time` (to 200)
- [ ] Decrease `sharp_turn_forward_time` (to 100)
- [ ] Check turn motor speed is adequate
- [ ] Verify other sensors are not also active (should be only s[0])

**Serial Monitor Check:**
```
Approaching turn:
S: 1 0 0 0 0 0 | SHARP RIGHT
(only rightmost sensor active, triggers sharp turn)
```

---

### Test 4: Sharp Left Turn (90°)
**Track:** 90° left angle turn

**Expected Behavior:**
1. Left edge sensor (s5) detects turn
2. Robot moves forward ~150ms
3. Robot spins left in place
4. Continues when line found

**Checklist:**
- [ ] Robot detects turn
- [ ] Robot moves forward before turning
- [ ] Robot spins left
- [ ] Robot finds line after turn
- [ ] Continues following after turn

**If fails:** Same troubleshooting as sharp right turn

**Serial Monitor Check:**
```
Approaching turn:
S: 0 0 0 0 0 1 | SHARP LEFT
(only leftmost sensor active)
```

---

### Test 5: T-Section (Junction)
**Track:** T-junction or cross intersection

**Expected Behavior:**
1. All 6 sensors detect line
2. Robot stops
3. Waits 150ms
4. Turns left or right (alternates)
5. Continues on new line

**First T-Section:**
- [ ] All sensors detect (sum=6)
- [ ] Robot stops at junction
- [ ] Robot turns LEFT (first turn is left by default)
- [ ] Robot finds new line
- [ ] Continues following

**Second T-Section:**
- [ ] Robot stops at junction
- [ ] Robot turns RIGHT (alternates from previous)
- [ ] Robot finds new line
- [ ] Continues following

**Third T-Section:**
- [ ] Robot turns LEFT again (alternating works)

**If fails:**
- [ ] Check all 6 sensors detect at junction (serial: sum=6)
- [ ] Increase junction confirm delay if false triggers
- [ ] Increase turn delay if doesn't complete 90°
- [ ] Check track width covers all sensors

**Serial Monitor Check:**
```
At junction:
S: 1 1 1 1 1 1 | sum=6 | T-TURN LEFT
(all sensors active, confirms T-section)
```

---

### Test 6: Dead-End (U-Turn)
**Track:** Line ending in solid black area

**Expected Behavior:**
1. Robot reaches black wall (all sensors black)
2. Robot stops
3. Waits 150ms (moves slightly forward)
4. All sensors now white (off the line)
5. Robot spins 180° (U-turn)
6. Robot finds line again
7. Continues in opposite direction

**Checklist:**
- [ ] Robot detects dead-end (all black → all white)
- [ ] Robot executes U-turn
- [ ] U-turn is approximately 180°
- [ ] Robot finds line after U-turn
- [ ] Continues following in opposite direction
- [ ] Doesn't immediately trigger T-section (just_uturn flag works)

**If fails:**
- [ ] Check dead-end has enough black area for all sensors
- [ ] Increase U-turn delay if doesn't complete 180°
- [ ] Check battery isn't too weak for sustained spin
- [ ] Verify just_uturn flag prevents false T-detection

**Serial Monitor Check:**
```
Approaching dead-end:
S: 1 1 1 1 1 1 | sum=6
(after delay)
S: 0 0 0 0 0 0 | sum=0 | U-TURN!
```

---

### Test 7: Lost Line Recovery
**Track:** Temporarily lift robot off track

**Expected Behavior:**
1. All sensors read white (sum=0)
2. Robot continues in last direction
3. If placed back on line, resumes following

**Checklist:**
- [ ] Robot doesn't stop when losing line
- [ ] Robot continues forward briefly
- [ ] Robot recovers if line is straight ahead
- [ ] Robot doesn't crash or behave erratically

**If fails:**
- [ ] Normal - basic lost line recovery is limited
- [ ] Can add search pattern if needed (spin to find line)

---

### Test 8: Motor Ramping (Smooth Speed Changes)
**Track:** Straight line with varying PID corrections

**Expected Behavior:**
- Robot accelerates smoothly (no jerks)
- Robot decelerates smoothly (no jerks)
- Speed changes are gradual

**Checklist:**
- [ ] No jerky movements during acceleration
- [ ] No jerky movements during deceleration
- [ ] Speed changes are smooth and gradual
- [ ] Robot doesn't overshoot corrections

**If fails:**
- [ ] Check motor ramp code has correct subtraction (see BUG_FIXES_SUMMARY)
- [ ] Adjust `rate` parameter (higher=faster changes, lower=smoother)
- [ ] Check motors aren't slipping on surface

**Serial Monitor Check:**
```
Watch motor values change gradually:
L=100 R=100
L=112 R=108  (ramping)
L=124 R=96   (ramping)
L=130 R=90   (approaching target)
```

---

## Advanced Tests

### Test 9: Complete Track
**Track:** Full course with all elements

- [ ] Straight sections: Follows smoothly
- [ ] Gentle curves: Handles without issue
- [ ] Sharp turns: Detects and completes
- [ ] T-sections: Alternates correctly
- [ ] Dead-ends: U-turns successfully
- [ ] Completes full course without human intervention
- [ ] Consistent performance over multiple runs

**Time to complete:** _____ seconds (benchmark for tuning)

---

### Test 10: Stress Test
**Track:** Complex course, multiple runs

- [ ] Run 1: Successful
- [ ] Run 2: Successful
- [ ] Run 3: Successful
- [ ] Run 4: Successful
- [ ] Run 5: Successful
- [ ] Consistent behavior across all runs
- [ ] No random failures
- [ ] Battery level doesn't cause issues

---

## Tuning Record

Use this space to record your final working values:

### Final Parameters:
```cpp
int threshold = _____;
int lbase = _____;
int rbase = _____;
int kp = _____;
int kd = _____;
int rate = _____;
int pwm_cap = _____;
int sharp_turn_forward_time = _____;
```

### Turn Delays:
```cpp
U-turn delay = ______ ms
Left turn delay = ______ ms
Right turn delay = ______ ms
Junction confirm delay = ______ ms
```

### Notes:
```
Best speed for your robot: _____
Best KP value: _____
Best KD value: _____
Track surface type: __________
Line width: _____ cm
Line color: __________
Background color: __________
Battery type: __________
Battery voltage: _____ V
```

---

## Common Problems Quick Reference

| Problem | Check These | Typical Fix |
|---------|-------------|-------------|
| Wobbles on straight | KP too high | Decrease kp by 10-20 |
| Slow to correct | KP too low | Increase kp by 10-20 |
| Overshoots curves | KD too low | Increase kd by 20-50 |
| Sluggish on curves | KD too high | Decrease kd by 20-50 |
| Loses sharp turns | Forward time wrong | Adjust sharp_turn_forward_time |
| Doesn't detect T | Threshold wrong | Recalibrate sensors |
| U-turn incomplete | Delay too short | Increase U-turn delay |
| Jerky movements | Motor ramp broken | Check fixed code is uploaded |
| One side drifts | Motor imbalance | Adjust lbase or rbase |
| Too slow overall | Base speed low | Increase lbase/rbase by 20 |

---

## Success Criteria

Your robot is working correctly when:

✓ Follows straight lines smoothly (no excessive wobble)
✓ Handles gentle curves without losing line
✓ Detects and completes sharp 90° turns (both left and right)
✓ Recognizes T-sections and alternates turns
✓ Detects dead-ends and executes U-turns
✓ Movement is smooth (no jerks or sudden stops)
✓ Consistent performance over multiple runs
✓ Completes full complex track without human intervention

---

## Next Steps After Testing

Once all tests pass:

- [ ] Document your final parameter values (above)
- [ ] Take video of successful run
- [ ] Experiment with higher speeds
- [ ] Try more complex track layouts
- [ ] Add additional features (speed optimization, shortcut detection, etc.)
- [ ] Share your success! 🎉

---

**Good luck with your testing! 🤖🏁**

*Remember: Tuning takes time. Small adjustments and patience lead to great results!*
