# 🤖 Line Follower Quick Reference Card
**Print this and keep it handy while tuning your robot!**

---

## 🔴 The 3 Critical Bugs (FIXED)

| Bug | Your Code | Fixed Code |
|-----|-----------|------------|
| **Motor Ramp** | `actual - actual` ❌ | `actual - target` ✅ |
| **Sharp Turn** | Sets variable, no action ❌ | Detects + turns ✅ |
| **Junction** | Two conflicting checks ❌ | One unified check ✅ |

---

## 📊 Default Parameters

```cpp
// SENSORS
int threshold = 512;         // Adjust after calibration!

// SPEEDS  
int lbase = 120;             // Left motor base speed
int rbase = 120;             // Right motor base speed
int pwm_cap = 180;           // Max PWM (255 = full)

// PID
int kp = 40;                 // Proportional gain
int kd = 100;                // Derivative gain
int rate = 12;               // Motor ramp rate

// TURNS
int sharp_turn_forward_time = 150;  // ms before sharp turn
```

---

## 🔧 Quick Tuning Guide

### Problem → Solution

| Problem | Fix | Adjust |
|---------|-----|--------|
| Wobbles on straight | KP too high | Decrease kp by 10-20 |
| Slow to correct | KP too low | Increase kp by 10-20 |
| Overshoots curves | KD too low | Increase kd by 20-50 |
| Sluggish response | KD too high | Decrease kd by 20-50 |
| Too slow | Speed low | Increase lbase/rbase by 20 |
| Loses sharp turns | Forward time | Adjust sharp_turn_forward_time |
| Doesn't detect line | Threshold | Recalibrate sensors |
| Jerky movements | Motor ramp | Check fixed code uploaded |
| Drifts to one side | Motor imbalance | Adjust lbase or rbase |

---

## 📏 Sensor Calibration

**Step 1:** Place on WHITE → record values
**Step 2:** Place on BLACK → record values  
**Step 3:** `threshold = (white + black) / 2`

**Example:**
- White: 200
- Black: 800
- Threshold: `(200 + 800) / 2 = 500`

---

## 🎮 Sensor Layout

```
[s5] [s4] [s3] [s2] [s1] [s0]
 A5   A4   A3   A2   A1   A0
 +3   +2   +1   -1   -2   -3   ← Position weights

LEFT              RIGHT
```

---

## 🔍 Detection Patterns

| Sensors | Meaning | Action |
|---------|---------|--------|
| `0 0 1 1 0 0` | Centered | Straight |
| `0 0 0 1 1 0` | Right | Turn right (PID) |
| `0 1 1 0 0 0` | Left | Turn left (PID) |
| `1 0 0 0 0 0` | Sharp left edge | Sharp left turn |
| `0 0 0 0 0 1` | Sharp right edge | Sharp right turn |
| `1 1 1 1 1 1` | All sensors | T-section or dead-end |
| `0 0 0 0 0 0` | No sensors | Lost line |

---

## 🔄 PID Formula

```
error = avg_sensor_position
derivative = current_error - previous_error
PID = (kp × error) + (kd × derivative)

left_motor = lbase + PID
right_motor = rbase - PID
```

**Example:** Line to right (avg = -1.5)
- PID = 40×(-1.5) + 100×0 = -60
- Left = 120 + (-60) = 60
- Right = 120 - (-60) = 180
- **Result:** Right motor faster → turn right ✓

---

## 📍 Pin Connections

```cpp
Left Motor:  Forward=2, Backward=6, Speed=3
Right Motor: Forward=4, Backward=5, Speed=9
Sensors:     A0-A5 (A0=rightmost, A5=leftmost)
```

---

## ✅ Testing Checklist

- [ ] 1. Straight line following
- [ ] 2. Gentle curves
- [ ] 3. Sharp 90° right turn
- [ ] 4. Sharp 90° left turn
- [ ] 5. T-section (alternates)
- [ ] 6. Dead-end U-turn
- [ ] 7. Smooth motor ramping
- [ ] 8. Complete course

---

## 🎯 Typical Value Ranges

| Parameter | Range | Start With |
|-----------|-------|------------|
| threshold | 400-600 | 512 |
| lbase/rbase | 80-160 | 120 |
| kp | 20-80 | 40 |
| kd | 50-200 | 100 |
| rate | 8-20 | 12 |
| pwm_cap | 150-255 | 180 |
| sharp_forward | 100-250 | 150 |

---

## 🚨 Common Mistakes

❌ Threshold not calibrated  
❌ Sensors too high/low (ideal: 2-5mm)  
❌ Sensors not perpendicular to line  
❌ Old buggy code still uploaded  
❌ Base speed too high for track  
❌ Not testing incrementally  

---

## 📞 Quick Help

**Can't follow line at all:**
→ Calibrate threshold first!

**Follows but loses sharp turns:**
→ Check s[0]/s[5] in serial monitor

**Stops at random:**
→ False junction detection, increase threshold

**Jerky movements:**
→ Make sure FIXED code is uploaded (motor ramp bug)

**One motor weaker:**
→ Adjust lbase or rbase (e.g., lbase=120, rbase=125)

---

## 🎓 Remember

1. **Start slow** - Begin with lbase=100
2. **Tune PID first** - Get straight line smooth
3. **Test incrementally** - One feature at a time
4. **Use serial monitor** - Watch sensor values
5. **Be patient** - Small adjustments add up!

---

## 📂 File Guide

**Upload this:** `line_follower_FIXED.ino`  
**Read first:** `FINAL_SUMMARY.md`  
**When tuning:** `TUNING_GUIDE.md`  
**When testing:** `TESTING_CHECKLIST.md`  
**Sensor help:** `SENSOR_ARRAY_REFERENCE.md`

---

## 💡 Pro Tips

- Test on **simple tracks first** (straight → curves → turns)
- Keep **battery charged** (weak battery = slow motors)
- **Watch serial monitor** while testing
- **Mark threshold** on track to remember it
- **Document** what works (write on this card!)
- **Iterate** - tune, test, adjust, repeat

---

## ✍️ Your Working Values

Fill in after successful tuning:

```
threshold = _______
lbase = _______
rbase = _______
kp = _______
kd = _______
rate = _______
sharp_turn_forward_time = _______

Notes:
_________________________________
_________________________________
_________________________________
_________________________________
```

---

## 🏁 Success = All These Work

✅ Smooth straight line following  
✅ No excessive wobbling  
✅ Handles curves gracefully  
✅ Completes sharp 90° turns  
✅ Detects T-sections correctly  
✅ Executes U-turns at dead-ends  
✅ Smooth acceleration/deceleration  
✅ Consistent over multiple runs  

---

**Good luck! 🤖🎉**

*Keep this card with you while tuning. Write notes directly on it!*

---

**IMPORTANT:** Upload `line_follower_FIXED.ino`, not your original code!
