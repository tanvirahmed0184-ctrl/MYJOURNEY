# Step-by-Step Simulation: Line Follower Robot

## Visual Representation Key
```
Sensor Array: [s0][s1][s2][s3][s4][s5]
               -3  -2  -1   1   2   3
                L   L   L   R   R   R
```

---

## SIMULATION 1: Straight Path Following

### Initial State
```
Sensors: [0][0][1][1][0][0]
         └─┴─┴─┴─┴─┴─┘
         Line centered
```

### Step-by-Step Execution

**Step 1: Reading()**
- `s[0]=0, s[1]=0, s[2]=1, s[3]=1, s[4]=0, s[5]=0`
- `sensor_pos = 0*(-3) + 0*(-2) + 1*(-1) + 1*(1) + 0*(2) + 0*(3) = 0`
- `sum = 2`

**Step 2: Turn Detection**
- `s[0] = 0`, `s[5] = 0` → No edge bias
- `turn = 's'` (straight)
- `last_turn` unchanged (memory preserved)

**Step 3: Junction Check**
- `sum == 6?` → NO
- Continue to PID

**Step 4: Sharp Turn Check**
- `s[0] && sum <= 2 && !s[5]?` → NO (s[0]=0)
- `s[5] && sum <= 2 && !s[0]?` → NO (s[5]=0)
- Continue to PID

**Step 5: PID Calculation**
- `avg = sensor_pos / sum = 0 / 2 = 0.0`
- `error[0] = 0.0`
- `derivative = error[0] - error[1] = 0.0 - 0.0 = 0.0`
- `PID = 50 * 0.0 + 120 * 0.0 = 0.0`
- `error[1] = 0.0` (save for next iteration)

**Step 6: Motor Calculation**
- `lmotor_target = 120 + 0 = 120`
- `rmotor_target = 120 - 0 = 120`
- Motor ramping: `lmotor_actual = 120`, `rmotor_actual = 120`

**Step 7: Motor Output**
- `motor(120, 120)` → Both forward at 120 PWM

**Result:** ✅ Robot moves straight forward

---

## SIMULATION 2: Right Curve (Moderate)

### Initial State
```
Sensors: [0][0][0][1][1][1]
         └─┴─┴─┴─┴─┴─┘
         Line shifted RIGHT
```

### Step-by-Step Execution

**Step 1: Reading()**
- `sensor_pos = 0 + 0 + 0 + 1 + 2 + 3 = 6`
- `sum = 3`

**Step 2: Turn Detection**
- `s[5] = 1`, `s[0] = 0` → Right bias detected
- `turn = 'r'`
- `last_turn = 'r'` (memory updated)

**Step 3: PID Calculation**
- `avg = 6 / 3 = 2.0`
- `error[0] = 2.0` (line is to the right)
- Assume `error[1] = 0.0` (previous was centered)
- `derivative = 2.0 - 0.0 = 2.0`
- `PID = 50 * 2.0 + 120 * 2.0 = 100 + 240 = 340`
- Constrain: `PID = 150` (max limit)

**Step 4: Motor Calculation**
- `lmotor_target = 120 + 150 = 270` → constrained to 180
- `rmotor_target = 120 - 150 = -30` → **NEGATIVE!**

**Step 5: Motor Ramping**
- `lmotor_actual` increases from 120 → 132 → 144 → ... → 180
- `rmotor_actual` decreases from 120 → 108 → 96 → ... → -30

**Step 6: Motor Output**
- Left motor: Forward at 180 PWM
- Right motor: **Reverse at 30 PWM** (bidirectional!)

**Result:** ✅ Robot pivots right sharply to catch line

---

## SIMULATION 3: Sharp 90° Right Turn

### Phase 1: Detection

**Initial State:**
```
Sensors: [1][0][0][0][0][0]
         └─┴─┴─┴─┴─┴─┘
         Only LEFT sensor detects!
```

**Step 1: Reading()**
- `sum = 1` (≤ 2) ✅
- `s[0] = 1` ✅
- `s[5] = 0` ✅

**Step 2: Sharp Turn Detection**
- Condition: `(s[0] && sum <= 2 && !s[5])` → **TRUE**
- `turn_right = true`

**Step 3: Forward Movement**
- `motor(120, 120)` → Move forward
- `delay(150)` → Wait 150ms
- Purpose: Move robot forward before turning

**Step 4: Execute Sharp Turn**
- Call `do_sharp_turn_right()`

### Phase 2: Turn Execution

**Step 5: Start Pivot**
- `motor(100, -100)` → Left forward, right reverse
- Robot pivots in place (right turn)

**Step 6: Loop Until Line Found**
```
Iteration 1:
Sensors: [0][0][0][0][0][0] → sum=0 → Continue pivoting

Iteration 2:
Sensors: [0][0][0][0][0][0] → sum=0 → Continue pivoting

Iteration 3:
Sensors: [0][0][1][0][0][0] → sum=1 → EXIT LOOP
```

**Step 7: Stop**
- `motor(0, 0)`
- `delay(50)`

**Result:** ✅ Sharp turn completed, line reacquired

---

## SIMULATION 4: T-Section (Left Turn)

### Phase 1: Detection

**Initial State:**
```
Sensors: [1][1][1][1][1][1]
         └─┴─┴─┴─┴─┴─┘
         ALL BLACK (junction)
```

**Step 1: Reading()**
- `sum = 6` ✅
- `just_junction = false` ✅

**Step 2: Junction Detection**
- `sum == 6 && !just_junction` → **TRUE**
- `motor(0, 0)` → **STOP**
- `delay(150)` → Wait and re-check

**Step 3: Re-read After Delay**
```
Sensors: [1][1][1][1][0][0]
         └─┴─┴─┴─┴─┴─┘
         Line continues LEFT
```

**Step 4: Verify Junction Type**
- `sum = 4` (≥ 3) → T-section detected ✅
- `sum != 6` → Not a black wall ✅

**Step 5: Choose Turn Direction**
- Check `last_T_turn`:
  - If `'l'` → Turn right, set `last_T_turn = 'r'`
  - If `'r'` → Turn left, set `last_T_turn = 'l'`
- Assume `last_T_turn = 'r'` → Turn left

**Step 6: Execute Turn**
- Call `do_turn_left()`

### Phase 2: Turn Execution

**Step 7: Initial Pivot**
- `motor(-100, 100)` → Pivot left
- `delay(350)` → Initial pivot time

**Step 8: Fine-tune Until Centered**
```
Iteration 1:
Sensors: [0][0][0][0][0][0] → s[2]=0, s[3]=0 → Continue

Iteration 2:
Sensors: [0][0][0][0][0][0] → Continue pivoting

Iteration 3:
Sensors: [0][0][1][0][0][0] → s[2]=1 → EXIT LOOP
```

**Step 9: Stop**
- `motor(0, 0)`
- `delay(50)`
- `just_junction = true` (prevent re-trigger)

**Result:** ✅ T-turn completed, robot on new path

---

## SIMULATION 5: Lost Line Recovery

### Initial State
```
Sensors: [0][0][0][0][0][0]
         └─┴─┴─┴─┴─┴─┘
         ALL WHITE (line lost)
```

**Previous State:** Robot was turning left (`last_turn = 'l'`)

### Step-by-Step Execution

**Step 1: Reading()**
- `sum = 0` → Line lost!

**Step 2: Lost Line Detection**
- `sum == 0` → Enter recovery mode

**Step 3: Check Memory**
- `last_turn = 'l'` (from previous reading)
- Print: `"LOST LINE - searching LEFT"`

**Step 4: Pivot Left**
- `motor(-120, 120)` → Left reverse, right forward
- Robot pivots left (searching)

**Step 5: Loop Until Line Found**
```
Iteration 1:
Sensors: [0][0][0][0][0][0] → sum=0 → Continue pivoting

Iteration 2:
Sensors: [0][0][0][0][0][0] → sum=0 → Continue pivoting

Iteration 3:
Sensors: [0][0][1][0][0][0] → sum=1 → EXIT RECOVERY
```

**Step 6: Resume PID**
- `sum > 0` → Return to normal PID following
- Robot continues on line

**Result:** ✅ Line recovered using memory-based search

---

## SIMULATION 6: Black Wall / End of Course

### Phase 1: Detection

**Initial State:**
```
Sensors: [1][1][1][1][1][1]
         └─┴─┴─┴─┴─┴─┘
         ALL BLACK
```

**Step 1: Reading()**
- `sum = 6`

**Step 2: Junction Detection**
- `motor(0, 0)` → Stop
- `delay(150)` → Wait

**Step 3: Re-read**
```
Sensors: [1][1][1][1][1][1]
         └─┴─┴─┴─┴─┴─┘
         STILL ALL BLACK
```

**Step 4: Black Wall Detection**
- `sum == 6` (after delay) → **BLACK WALL**
- Print: `"BLACK WALL - END! STOPPING."`
- `motor(0, 0)` → Stop motors
- `while(1)` → **INFINITE LOOP** (robot stops forever)

**Result:** ✅ Robot stops permanently (end of course)

---

## SIMULATION 7: Cross-Section (BUG!)

### Phase 1: Detection

**Initial State:**
```
Sensors: [1][1][1][1][1][1]
         └─┴─┴─┴─┴─┴─┘
         ALL BLACK (4-way junction)
```

**Step 1-3:** Same as T-section detection

**Step 4: Re-read After Delay**
```
Sensors: [1][1][1][1][1][1]
         └─┴─┴─┴─┴─┴─┘
         STILL ALL BLACK (cross-section)
```

**Step 5: Black Wall Detection**
- `sum == 6` → Treated as BLACK WALL ❌
- Robot stops permanently ❌

**Expected Behavior:** Should choose direction (forward/left/right)
**Actual Behavior:** Stops forever (BUG!)

---

## SIMULATION 8: Zigzag Path

### Pattern: Left → Right → Left → Right

**State 1: Line Shifts Left**
```
Sensors: [1][1][1][0][0][0]
```
- `avg = -2.0`, `PID = -100`
- Robot turns left, `last_turn = 'l'`

**State 2: Line Shifts Right**
```
Sensors: [0][0][0][1][1][1]
```
- `avg = 2.0`, `PID = 100`
- Robot turns right, `last_turn = 'r'`

**State 3: Sharp Left Turn**
```
Sensors: [1][0][0][0][0][0]
```
- Sharp turn detected → Execute `do_sharp_turn_left()`

**State 4: Return to Center**
```
Sensors: [0][0][1][1][0][0]
```
- Normal PID following resumes

**Result:** ✅ Robot handles zigzag with PID + sharp turns

---

## SIMULATION 9: 45-Degree Turn (Bidirectional in Action)

### Initial State
```
Sensors: [0][0][0][0][1][1]
         └─┴─┴─┴─┴─┴─┘
         Line at far right
```

**Step 1: Reading()**
- `sum = 2`
- `sensor_pos = 2 + 3 = 5`
- `avg = 5 / 2 = 2.5`

**Step 2: Sharp Turn Check**
- `sum = 2` (not ≤ 2 with edge condition) → NO
- Continue to PID

**Step 3: PID Calculation**
- `error[0] = 2.5`
- `PID = 50 * 2.5 = 125` (assuming small derivative)

**Step 4: Motor Calculation**
- `lmotor_target = 120 + 125 = 245` → constrained to 180
- `rmotor_target = 120 - 125 = -5` → **NEGATIVE!**

**Step 5: Motor Output**
- Left motor: Forward at 180 PWM
- Right motor: **Reverse at 5 PWM** (bidirectional!)

**Result:** ✅ Robot makes tight turn using bidirectional control

---

## Summary Table

| Scenario | Detection | Action | Motor Output | Result |
|----------|-----------|--------|--------------|--------|
| **Straight** | `sum=2, avg=0` | PID | `(120, 120)` | ✅ Forward |
| **Right Curve** | `sum=3, avg=2.0` | PID | `(180, -30)` | ✅ Turn right |
| **Left Curve** | `sum=3, avg=-2.0` | PID | `(-30, 180)` | ✅ Turn left |
| **Sharp Right** | `s[0]=1, sum≤2` | Sharp turn | `(100, -100)` | ✅ Pivot right |
| **Sharp Left** | `s[5]=1, sum≤2` | Sharp turn | `(-100, 100)` | ✅ Pivot left |
| **T-Section** | `sum=6 → sum≥3` | T-turn | Pivot + search | ✅ Turn executed |
| **Black Wall** | `sum=6 → sum=6` | Stop | `(0, 0)` + `while(1)` | ✅ Permanent stop |
| **Cross-Section** | `sum=6 → sum=6` | Stop | `(0, 0)` + `while(1)` | ❌ **BUG!** |
| **Lost Line** | `sum=0` | Recovery | Pivot using memory | ✅ Line found |

---

## Critical Paths

### Path 1: Normal Following
```
Reading() → Turn Check → Junction Check → Sharp Check → PID → Motor
```

### Path 2: Sharp Turn
```
Reading() → Sharp Detection → Forward → do_sharp_turn_*() → Resume PID
```

### Path 3: Junction
```
Reading() → Junction Detection → Stop → Re-read → T-turn OR Black Wall
```

### Path 4: Lost Line
```
Reading() → sum=0 → Check last_turn → Pivot → Loop until sum>0
```
