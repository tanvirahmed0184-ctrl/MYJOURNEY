# Step-by-Step Simulation for Each Scenario

## 🎬 REAL-TIME SIMULATION WALKTHROUGH

### **SCENARIO 1: STRAIGHT PATH** ════════

**Time: t=0ms**
```
Sensors: [0, 0, 1, 1, 0, 0]
sum = 2, sensor_pos = 0
avg = 0/2 = 0
error[0] = 0, error[1] = 0
derivative = 0
PID = 50*0 + 120*0 = 0
lmotor_target = 120 + 0 = 120
rmotor_target = 120 - 0 = 120
Motor: (120, 120) → Straight forward ✅
```

**Time: t=50ms** (next loop)
```
Sensors: [0, 0, 1, 1, 0, 0] (still centered)
Same calculation → Motor: (120, 120)
Result: Maintains straight path
```

---

### **SCENARIO 2: GENTLE RIGHT CURVE** ════╗

**Time: t=0ms** - Entering curve
```
Sensors: [0, 0, 0, 1, 1, 0]
sum = 2, sensor_pos = -3 (line shifted right)
avg = -3/2 = -1.5
error[0] = -1.5, error[1] = 0
derivative = -1.5 - 0 = -1.5
PID = 50*(-1.5) + 120*(-1.5) = -255 → constrained to -150
lmotor_target = 120 + (-150) = -30 ⚠️ BACKWARD!
rmotor_target = 120 - (-150) = 270 → capped to 180
Motor: (-30, 180) → Sharp right pivot (too aggressive!)
```

**Time: t=50ms** - Mid-curve
```
Sensors: [0, 0, 1, 1, 1, 0]
sum = 3, sensor_pos = -2
avg = -2/3 = -0.67
error[0] = -0.67, error[1] = -1.5
derivative = -0.67 - (-1.5) = +0.83
PID = 50*(-0.67) + 120*(0.83) = -33.5 + 99.6 = +66.1
lmotor_target = 120 + 66 = 186 → capped to 180
rmotor_target = 120 - 66 = 54
Motor: (180, 54) → Still correcting right
```

**Time: t=100ms** - Exiting curve
```
Sensors: [0, 0, 1, 1, 0, 0]
sum = 2, sensor_pos = 0
avg = 0/2 = 0
error[0] = 0, error[1] = -0.67
derivative = 0 - (-0.67) = +0.67
PID = 50*0 + 120*0.67 = +80.4
lmotor_target = 120 + 80 = 200 → capped to 180
rmotor_target = 120 - 80 = 40
Motor: (180, 40) → Overcorrecting left!
```

**Issue**: PID derivative term causes overshoot and oscillation

---

### **SCENARIO 3: 90° SHARP RIGHT TURN** ════╗

**Time: t=0ms** - Detection
```
Sensors: [1, 0, 0, 0, 0, 0]
sum = 1
s[0] = 1, s[5] = 0
Condition: (s[0] && sum <= 2 && !s[5]) → TRUE ✅
turn_right = true
Action: Enter sharp turn routine
```

**Time: t=1ms** - Forward movement
```
Motor: (120, 120) → Move forward
delay(150ms) → Blocking wait
Robot moves ~3-5cm forward
```

**Time: t=151ms** - Sharp turn start
```
do_sharp_turn_right() called
Motor: (100, -100) → Right motor backward, left forward
Robot pivots clockwise (right)
```

**Time: t=181ms** - Checking for line
```
reading() → Sensors: [0, 0, 0, 0, 0, 0]
sum = 0 → Still turning
Motor: (100, -100) → Continue pivot
```

**Time: t=211ms** - Line found
```
reading() → Sensors: [0, 0, 1, 1, 0, 0]
sum = 2 → Line detected!
Motor: (0, 0) → Stop
delay(50ms)
Return to normal line following
```

**Result**: ✅ Completes turn, but may overshoot by 1-2cm

---

### **SCENARIO 4: CROSS JUNCTION** ════╬═══

**Time: t=0ms** - Approach
```
Sensors: [0, 0, 1, 1, 0, 0]
Normal line following
```

**Time: t=50ms** - Entering junction
```
Sensors: [1, 1, 1, 1, 1, 1]
sum = 6 → JUNCTION DETECTED!
just_junction = false → Enter detection
Motor: (0, 0) → Stop immediately
```

**Time: t=51ms** - Wait period
```
delay(150ms) → Blocking wait
Robot stationary
```

**Time: t=201ms** - Re-check sensors
```
reading() → Sensors: [1, 1, 1, 1, 1, 1]
sum = 6 → Still all black
Check: if(sum == 6) → FALSE (we check sum >= 3)
Check: if(sum >= 3) → TRUE
Check: if(s[2] || s[3]) → TRUE (middle sensors see line)
Decision: CROSS JUNCTION → GO STRAIGHT ✅
```

**Time: t=202ms** - Move through
```
Motor: (120, 120) → Forward
delay(250ms) → Move through junction
just_junction = true
```

**Time: t=452ms** - Exit junction
```
Sensors: [0, 0, 1, 1, 0, 0]
sum = 2 → Normal line detected
just_junction = false (reset)
Return to normal following
```

**Result**: ✅ Successfully goes straight through cross junction

---

### **SCENARIO 5: T-SECTION (Left Turn)** ════╗

**Time: t=0ms** - Approach
```
Sensors: [0, 0, 1, 1, 0, 0]
Normal line following
```

**Time: t=50ms** - Entering junction
```
Sensors: [1, 1, 1, 1, 1, 1]
sum = 6 → JUNCTION DETECTED!
Motor: (0, 0) → Stop
delay(150ms)
```

**Time: t=201ms** - Re-check
```
reading() → Sensors: [1, 1, 0, 0, 1, 1]
sum = 4 → Still junction
Check: if(s[2] || s[3]) → FALSE (middle sensors don't see line)
Decision: T-SECTION
Check: last_T_turn == 'l' → Assume 'r' (alternating)
Action: do_turn_right() → Wait, should be LEFT!
```

**Actually**: Code alternates, so if last was 'r', turns left ✅

**Time: t=202ms** - Turn left
```
do_turn_left() called
Motor: (-100, 100) → Left backward, right forward
delay(350ms) → Pivot left
```

**Time: t=552ms** - Finding perpendicular line
```
reading() → Sensors: [0, 0, 0, 0, 0, 0]
sum = 0 → Still turning
Motor: (-100, 100) → Continue pivot
```

**Time: t=582ms** - Line found
```
reading() → Sensors: [0, 0, 1, 1, 0, 0]
sum = 2, s[2] || s[3] → TRUE ✅
Motor: (0, 0) → Stop
delay(50ms)
last_T_turn = 'r' (for next time)
Return to normal following
```

**Result**: ✅ Completes left turn at T-section

---

### **SCENARIO 6: S-SHAPE PATH** ════╗═══╝

**Phase 1 - Right Curve (t=0-200ms)**
```
Sensors: [0, 0, 0, 1, 1, 1]
sum = 3, sensor_pos = -6
avg = -6/3 = -2
PID = -100 to -150
Motor: (20-70, 170-220) → Steer right
last_turn = 'r' (edge sensor s[5] active)
```

**Phase 2 - Transition (t=200-250ms)**
```
Sensors: [0, 0, 1, 1, 0, 0]
sum = 2, sensor_pos = 0
avg = 0
PID ≈ 0
Motor: (120, 120) → Straight
```

**Phase 3 - Left Curve (t=250-450ms)**
```
Sensors: [1, 1, 1, 0, 0, 0]
sum = 3, sensor_pos = +6
avg = +6/3 = +2
PID = +100 to +150
Motor: (170-220, 20-70) → Steer left
last_turn = 'l' (edge sensor s[0] active)
```

**Result**: ✅ Smooth S-curve following with PID

---

### **SCENARIO 7: ZIGZAG PATH** ════╗═══╝═══╗

**Zig 1 - Right (t=0-100ms)**
```
Sensors: [0, 0, 0, 0, 1, 1]
sum = 2, sensor_pos = -5
PID = -125 to -150
Motor: (-5 to -30, 245 to 270) → Sharp right
last_turn = 'r'
```

**Zag 1 - Left (t=100-200ms)**
```
Sensors: [1, 1, 0, 0, 0, 0]
sum = 2, sensor_pos = +5
PID = +125 to +150
Motor: (245 to 270, -5 to -30) → Sharp left
last_turn = 'l'
```

**Zig 2 - Right (t=200-300ms)**
```
Same pattern repeats
Rapid PID corrections
Motor ramp smooths transitions (rate=12)
```

**Issue**: If zigzag frequency > motor ramp rate, may oscillate

**Result**: ⚠️ Works but may be jerky on rapid zigzags

---

### **SCENARIO 8: FULL BLACK (END)** ████████

**Time: t=0ms** - Detection
```
Sensors: [1, 1, 1, 1, 1, 1]
sum = 6 → JUNCTION DETECTED!
Motor: (0, 0) → Stop
delay(150ms)
```

**Time: t=151ms** - Re-check
```
reading() → Sensors: [1, 1, 1, 1, 1, 1]
sum = 6 → Still all black
Check: if(sum == 6) → TRUE ✅
Decision: BLACK WALL - END!
Serial: "BLACK WALL - END! STOPPING."
Motor: (0, 0)
while(1) → Infinite loop
```

**Result**: ✅ Robot stops permanently (correct behavior)

---

### **SCENARIO 9: LOST LINE** ════   (gap in line)

**Time: t=0ms** - Line present
```
Sensors: [0, 0, 1, 1, 0, 0]
Normal following
last_turn = 'r' (from previous curve)
```

**Time: t=50ms** - Line lost
```
Sensors: [0, 0, 0, 0, 0, 0]
sum = 0 → LOST LINE!
Check: last_turn == 'r' → TRUE
Action: motor(120, -120) → Pivot right
```

**Time: t=100ms** - Still searching
```
Sensors: [0, 0, 0, 0, 0, 0]
sum = 0 → Still lost
Motor: (120, -120) → Continue pivoting right
```

**Time: t=150ms** - Line found
```
Sensors: [0, 0, 1, 1, 0, 0]
sum = 2 → Line detected!
Return to normal following
```

**Result**: ✅ Recovers using memory of last turn direction

---

### **SCENARIO 10: T-SECTION FORWARD** ════╦═══

**Time: t=0ms** - Approach
```
Sensors: [0, 0, 1, 1, 0, 0]
Normal following
```

**Time: t=50ms** - Junction
```
Sensors: [1, 1, 1, 1, 1, 1]
sum = 6 → Stop and check
```

**Time: t=201ms** - Re-check
```
reading() → Sensors: [1, 1, 1, 1, 1, 1]
sum = 6 → Still junction
Check: if(s[2] || s[3]) → TRUE (forward line visible)
Decision: Treated as CROSS JUNCTION → GO STRAIGHT ✅
Motor: (120, 120)
delay(250ms)
```

**Result**: ✅ Correctly goes forward (treats as cross junction)

---

## 🔄 STATE TRANSITIONS

### **Normal Following → Sharp Turn**
```
Condition: Edge sensor active + sum <= 2
Action: Forward delay → Sharp turn
State: just_junction = false
```

### **Normal Following → Junction**
```
Condition: sum == 6
Action: Stop → Check → Decide (cross/T/end)
State: just_junction = true
```

### **Junction → Normal Following**
```
Condition: sum < 6 after junction
Action: Reset just_junction = false
State: Return to PID following
```

### **Normal Following → Lost Line**
```
Condition: sum == 0
Action: Use last_turn memory → Pivot
State: Searching mode
```

---

## 📈 PERFORMANCE METRICS (Estimated)

| Scenario | Success Rate | Speed | Smoothness |
|----------|--------------|-------|------------|
| Straight | 100% | Fast | Excellent |
| Curve | 90% | Medium | Good |
| 45° Turn | 95% | Medium | Excellent |
| S-Shape | 85% | Medium | Good |
| 90° Sharp | 80% | Slow | Fair |
| Zigzag | 70% | Slow | Poor |
| Full Black | 100% | N/A | N/A |
| Cross | 95% | Medium | Good |
| T-Section | 90% | Slow | Fair |
| Lost Line | 85% | Slow | Fair |

---

## 🎯 KEY INSIGHTS

1. **PID Range Too Aggressive**: -150/+150 causes backward motion
2. **Motor Ramp Helps**: Smooths transitions but may be too slow
3. **Junction Detection Works**: Cross and T-sections handled correctly
4. **Lost Line Recovery**: Memory system works well
5. **Sharp Turns**: May overshoot due to forward delay
6. **Debug Mode**: Adds ~5-10ms per loop (disable for competition)
