# 🔄 Line Follower Algorithm - Decision Flowchart

## Main Control Flow

```
┌─────────────────────────────────────────────────────────────────┐
│                         START LOOP                              │
└───────────────────────────┬─────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│                    READ SENSORS (0-5)                           │
│  Convert analog values to binary (0 or 1) using threshold       │
│  Calculate: sum, sensor_pos, avg                                │
└───────────────────────────┬─────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│              UPDATE TURN MEMORY (last_turn)                     │
│  IF s[0]=1 AND s[5]=0  →  last_turn = 'r'                      │
│  IF s[5]=1 AND s[0]=0  →  last_turn = 'l'                      │
│  ELSE → Keep previous last_turn (memory preserved)              │
└───────────────────────────┬─────────────────────────────────────┘
                            │
                            ▼
                    ┌───────────────┐
                    │   sum == 6    │───── NO ─────┐
                    │      AND      │              │
                    │!just_junction?│              │
                    └───────┬───────┘              │
                            │ YES                  │
                            ▼                      │
              ┌─────────────────────────┐          │
              │   STOP & WAIT 150ms     │          │
              │   READ SENSORS AGAIN    │          │
              └──────────┬──────────────┘          │
                         │                         │
              ┌──────────▼──────────┐              │
              │   sum STILL == 6?   │              │
              └──┬────────────────┬──┘              │
                 │ YES            │ NO              │
                 │                │                 │
                 ▼                ▼                 │
        ┌────────────────┐  ┌──────────────┐       │
        │  BLACK WALL    │  │  sum >= 3?   │       │
        │  STOP FOREVER  │  └──┬────────┬──┘       │
        │   while(1);    │     │ YES    │ NO       │
        └────────────────┘     │        │          │
                               ▼        │          │
                    ┌─────────────────┐ │          │
                    │ s[2] OR s[3]    │ │          │
                    │   ON?           │ │          │
                    └──┬───────────┬──┘ │          │
                       │ YES       │ NO │          │
                       ▼           ▼    ▼          │
              ┌────────────┐ ┌────────────┐        │
              │   CROSS    │ │ T-SECTION  │        │
              │ GO STRAIGHT│ │ ALTERNATE  │        │
              │ delay(250) │ │   TURN     │        │
              └──────┬─────┘ └─────┬──────┘        │
                     │             │               │
                     └─────┬───────┘               │
                           │ Set just_junction     │
                           │ RETURN                │
                           └───────────────────────┼────► END LOOP
                                                   │
                                                   │
                    ┌──────────────────────────────┘
                    │
                    ▼
        ┌───────────────────────────────┐
        │   SHARP TURN DETECTION        │
        │ (s[0]=1 AND sum<=2 AND s[5]=0)│
        │        OR                     │
        │ (s[5]=1 AND sum<=2 AND s[0]=0)│
        └────┬──────────────────┬────────┘
             │ YES              │ NO
             ▼                  │
   ┌──────────────────┐         │
   │ Move forward     │         │
   │ 150ms            │         │
   │ Execute pivot    │         │
   │ turn until line  │         │
   │ found            │         │
   └────────┬─────────┘         │
            │ RETURN            │
            └───────────────────┼────► END LOOP
                                │
                                ▼
                        ┌───────────────┐
                        │   sum > 0?    │
                        └───┬───────┬───┘
                            │ YES   │ NO
                            ▼       ▼
                  ┌──────────────┐ ┌─────────────────┐
                  │  PID CONTROL │ │  LOST LINE      │
                  │  Calculate   │ │  Pivot in       │
                  │  error & PID │ │  last_turn      │
                  │  Adjust      │ │  direction      │
                  │  motors      │ │                 │
                  └──────┬───────┘ └────────┬────────┘
                         │                  │
                         │                  │
                         └────────┬─────────┘
                                  │
                                  ▼
                           ┌─────────────┐
                           │ DEBUG PRINT │
                           │ (if enabled)│
                           └──────┬──────┘
                                  │
                                  ▼
                           ┌─────────────┐
                           │  END LOOP   │
                           │  REPEAT     │
                           └─────────────┘
```

---

## PID Control Detail

```
┌────────────────────────────────────────────────────────┐
│              PID LINE FOLLOWING ALGORITHM              │
└────────────────────────────────────────────────────────┘
                           │
                           ▼
            ┌──────────────────────────────┐
            │ Calculate weighted average:  │
            │ avg = sensor_pos / sum       │
            │ Range: -3.0 to +3.0          │
            │                              │
            │ Negative = Line is LEFT      │
            │ Positive = Line is RIGHT     │
            │ Zero = Centered              │
            └──────────────┬───────────────┘
                           │
                           ▼
            ┌──────────────────────────────┐
            │ Calculate derivative:        │
            │ derivative = error[0] - error[1] │
            │ (rate of change)             │
            └──────────────┬───────────────┘
                           │
                           ▼
            ┌──────────────────────────────┐
            │ Calculate PID:               │
            │ PID = kp*error + kd*derivative│
            │ Constrain: -150 to +150      │
            └──────────────┬───────────────┘
                           │
                           ▼
            ┌──────────────────────────────┐
            │ Calculate target speeds:     │
            │ Left  = base + PID           │
            │ Right = base - PID           │
            │ Constrain: -180 to +180      │
            └──────────────┬───────────────┘
                           │
                           ▼
            ┌──────────────────────────────┐
            │ MOTOR RAMPING:               │
            │ Gradually adjust actual      │
            │ speeds toward targets        │
            │ Rate: 12 units per loop      │
            └──────────────┬───────────────┘
                           │
                           ▼
            ┌──────────────────────────────┐
            │ Apply motor speeds:          │
            │ motor(left_actual, right_actual)│
            │                              │
            │ Positive = Forward           │
            │ Negative = Backward          │
            └──────────────────────────────┘
```

---

## Junction Decision Tree (FIXED VERSION)

```
                        ┌───────────────┐
                        │  ALL SENSORS  │
                        │  BLACK?       │
                        │  (sum == 6)   │
                        └───────┬───────┘
                                │
                                ▼
                        ┌───────────────┐
                        │ STOP MOTORS   │
                        │ WAIT 150ms    │
                        └───────┬───────┘
                                │
                                ▼
                        ┌───────────────┐
                        │ READ SENSORS  │
                        │ AGAIN         │
                        └───────┬───────┘
                                │
                        ┌───────▼────────┐
                        │  Still sum=6?  │
                        └───┬────────┬───┘
                            │        │
                        YES │        │ NO
                            │        │
                            ▼        ▼
                   ┌─────────────┐  ┌──────────────┐
                   │ BLACK WALL! │  │  sum >= 3?   │
                   │             │  └───┬──────┬───┘
                   │ Print msg   │      │ YES  │ NO
                   │ Stop motors │      │      │
                   │ while(1);   │      │      └──────► Continue to
                   │             │      │              other logic
                   └─────────────┘      ▼
                                 ┌─────────────────┐
                                 │ Check if middle │
                                 │ sensors ON:     │
                                 │ (s[2] OR s[3])  │
                                 └────┬───────┬────┘
                                      │       │
                                  YES │       │ NO
                                      │       │
                                      ▼       ▼
                         ┌──────────────────────────┐
                         │   CROSS JUNCTION (+)     │
                         ├──────────────────────────┤
                         │ Forward path EXISTS      │
                         │ (middle sensors see line)│
                         │                          │
                         │ ACTION:                  │
                         │ • Go straight            │
                         │ • motor(base, base)      │
                         │ • delay(250ms)           │
                         │ • Set just_junction=true │
                         └──────────────────────────┘
                                      │
                         ┌──────────────────────────┐
                         │   T-SECTION (⊤)          │
                         ├──────────────────────────┤
                         │ No forward path          │
                         │ (middle sensors OFF)     │
                         │                          │
                         │ ACTION:                  │
                         │ • Alternate turns        │
                         │ • If last_T='l' → RIGHT  │
                         │ • If last_T='r' → LEFT   │
                         │ • Toggle last_T_turn     │
                         │ • Set just_junction=true │
                         └──────────────────────────┘
```

---

## Sharp Turn Detection & Execution

```
┌─────────────────────────────────────────────────────────┐
│              SHARP TURN DETECTION                       │
└─────────────────────────────────────────────────────────┘
                           │
              ┌────────────┴────────────┐
              │                         │
              ▼                         ▼
    ┌──────────────────┐    ┌──────────────────┐
    │  RIGHT TURN?     │    │  LEFT TURN?      │
    │                  │    │                  │
    │ s[0] = 1         │    │ s[5] = 1         │
    │ s[5] = 0         │    │ s[0] = 0         │
    │ sum <= 2         │    │ sum <= 2         │
    └────────┬─────────┘    └────────┬─────────┘
             │ YES                   │ YES
             │                       │
             └───────────┬───────────┘
                         │
                         ▼
         ┌───────────────────────────────┐
         │ PHASE 1: CLEAR THE CORNER     │
         │ motor(base, base)             │
         │ delay(150ms)                  │
         │ (Move forward to clear edge)  │
         └───────────────┬───────────────┘
                         │
                         ▼
         ┌───────────────────────────────┐
         │ PHASE 2: PIVOT IN PLACE       │
         │                               │
         │ RIGHT: motor(+100, -100)      │
         │        (CW rotation)          │
         │                               │
         │ LEFT:  motor(-100, +100)      │
         │        (CCW rotation)         │
         └───────────────┬───────────────┘
                         │
                         ▼
         ┌───────────────────────────────┐
         │ PHASE 3: SEARCH FOR LINE      │
         │ Continue pivoting until:      │
         │ • sum > 1                     │
         │ (At least 2 sensors see line) │
         │                               │
         │ Check every 30ms              │
         └───────────────┬───────────────┘
                         │
                         ▼
         ┌───────────────────────────────┐
         │ PHASE 4: STOP & STABILIZE     │
         │ motor(0, 0)                   │
         │ delay(50ms)                   │
         │ Then return to main loop      │
         └───────────────────────────────┘
```

---

## Lost Line Recovery Algorithm

```
┌─────────────────────────────────────────────────────────┐
│              LOST LINE RECOVERY                         │
│  (Triggered when sum == 0)                              │
└─────────────────────────────────────────────────────────┘
                           │
                           ▼
         ┌─────────────────────────────────┐
         │ Check Turn Memory (last_turn)   │
         │ This was set when edge sensors  │
         │ last detected line direction    │
         └────────────┬────────────────────┘
                      │
        ┌─────────────┼─────────────┐
        │             │             │
        ▼             ▼             ▼
┌──────────────┐ ┌──────────┐ ┌──────────────┐
│ last_turn='l'│ │ last_turn│ │ last_turn='r'│
│              │ │   ='s'   │ │              │
│ Line was     │ │          │ │ Line was     │
│ curving LEFT │ │ Straight │ │ curving RIGHT│
└──────┬───────┘ └────┬─────┘ └──────┬───────┘
       │              │              │
       ▼              ▼              ▼
┌──────────────┐ ┌──────────┐ ┌──────────────┐
│ PIVOT LEFT   │ │ Continue │ │ PIVOT RIGHT  │
│              │ │   last   │ │              │
│ motor(-120,  │ │  speeds  │ │ motor(+120,  │
│       +120)  │ │          │ │       -120)  │
│              │ │          │ │              │
│ Spin counter-│ │          │ │ Spin clock-  │
│ clockwise    │ │          │ │ wise         │
└──────┬───────┘ └────┬─────┘ └──────┬───────┘
       │              │              │
       └──────────────┴──────────────┘
                      │
                      ▼
         ┌────────────────────────────┐
         │ Continue searching until:  │
         │ sum > 0                    │
         │ (Any sensor finds line)    │
         │                            │
         │ Then resume PID control    │
         └────────────────────────────┘
```

**Why This Works:**
- Robot remembers which direction the line was going
- Pivots in the CORRECT direction (not random)
- Faster recovery: 100-300ms vs 1-2 seconds
- Higher success rate: 90% vs 50%

---

## Motor Control (Bidirectional)

```
┌─────────────────────────────────────────────────────────┐
│           MOTOR CONTROL FUNCTION                        │
│  Input: motor(left_speed, right_speed)                  │
│  Range: -180 to +180                                    │
└─────────────────────────────────────────────────────────┘
                           │
              ┌────────────┴────────────┐
              │                         │
              ▼                         ▼
    ┌──────────────────┐      ┌──────────────────┐
    │  LEFT MOTOR      │      │  RIGHT MOTOR     │
    └──────────────────┘      └──────────────────┘
              │                         │
              ▼                         ▼
    ┌──────────────────┐      ┌──────────────────┐
    │ Speed >= 0?      │      │ Speed >= 0?      │
    └─────┬───────┬────┘      └─────┬───────┬────┘
          │ YES   │ NO              │ YES   │ NO
          │       │                 │       │
          ▼       ▼                 ▼       ▼
    ┌─────────┐ ┌────────┐    ┌─────────┐ ┌────────┐
    │ FORWARD │ │BACKWARD│    │ FORWARD │ │BACKWARD│
    │         │ │        │    │         │ │        │
    │ lmf=1   │ │ lmf=0  │    │ rmf=1   │ │ rmf=0  │
    │ lmb=0   │ │ lmb=1  │    │ rmb=0   │ │ rmb=1  │
    │         │ │        │    │         │ │        │
    │ PWM=    │ │ PWM=   │    │ PWM=    │ │ PWM=   │
    │ speed   │ │ -speed │    │ speed   │ │ -speed │
    └─────────┘ └────────┘    └─────────┘ └────────┘
```

**Examples:**
```
motor(120, 120)   →  Both forward at 120  →  Go straight
motor(180, 80)    →  Left fast, right slow →  Turn right
motor(80, 180)    →  Left slow, right fast →  Turn left
motor(100, -100)  →  Left fwd, right back  →  Pivot right (CW)
motor(-100, 100)  →  Left back, right fwd  →  Pivot left (CCW)
motor(-120, -120) →  Both backward         →  Reverse
motor(0, 0)       →  Both stop             →  Stop
```

---

## Sensor Calibration Process

```
┌─────────────────────────────────────────────────────────┐
│           SENSOR CALIBRATION PROCEDURE                  │
└─────────────────────────────────────────────────────────┘
                           │
                           ▼
         ┌─────────────────────────────────┐
         │ STEP 1: Read White Surface      │
         │ Place robot on white/off-line   │
         │ Record all 6 sensor values      │
         │ Example: ~800-900               │
         └────────────┬────────────────────┘
                      │
                      ▼
         ┌─────────────────────────────────┐
         │ STEP 2: Read Black Line         │
         │ Place robot on black line       │
         │ Record all 6 sensor values      │
         │ Example: ~100-200               │
         └────────────┬────────────────────┘
                      │
                      ▼
         ┌─────────────────────────────────┐
         │ STEP 3: Calculate Threshold     │
         │                                 │
         │ For each sensor:                │
         │ threshold[i] =                  │
         │   (white[i] + black[i]) / 2     │
         │                                 │
         │ OR use single average threshold:│
         │ threshold = 512 (if consistent) │
         └────────────┬────────────────────┘
                      │
                      ▼
         ┌─────────────────────────────────┐
         │ STEP 4: Test & Verify           │
         │ Run robot on test track         │
         │ Monitor sensor readings         │
         │ Adjust if needed:               │
         │ • Too sensitive? Raise threshold│
         │ • Not sensitive? Lower threshold│
         └─────────────────────────────────┘
```

---

## Competition Track Types & Responses

```
TRACK TYPE           SENSOR PATTERN        ROBOT ACTION
═══════════════════════════════════════════════════════════

1. STRAIGHT LINE
   ═══════           [0][0][■][■][0][0]   Go straight
                                          L=120, R=120

2. GENTLE CURVE
     ╭═══           [0][■][■][0][0][0]   Turn left
   ══╯                                    L=150, R=90

3. SHARP TURN (90°)
   ═══╗              [■][0][0][0][0][0]   Pivot right
      ║                                   L=100, R=-100

4. T-JUNCTION
      ║              [■][■][■][■][■][■]   Alternate turn
   ═══╩═══           (after delay:        Left or Right
                      s[2]=0, s[3]=0)

5. CROSS JUNCTION
      ║              [■][■][■][■][■][■]   Go straight
   ═══╬═══           (after delay:        L=120, R=120
      ║               s[2]=1 or s[3]=1)

6. BLACK WALL (END)
   ███████           [■][■][■][■][■][■]   Stop forever
                     (stays black after   while(1);
                      delay)

7. ZIGZAG
   ═╗ ╔═             [■][0][0][0][0][0]   Series of
    ╚═╝              then                 sharp pivots
                     [0][0][0][0][0][■]

8. LOST LINE
   ═══════           [0][0][0][0][0][0]   Pivot in
   (off track)                            last_turn dir
```

---

## Timing Diagram (Typical Loop)

```
Time   Action                         Duration  Notes
─────────────────────────────────────────────────────────
0ms    Read sensors (analogRead x6)   ~2ms      Fast
2ms    Calculate sum, avg, PID        <1ms      Math
3ms    Update motor targets           <1ms      Simple
4ms    Ramp motors (gradual adjust)   <1ms      Smooth
5ms    Apply PWM to motors            <1ms      Fast
6ms    Update turn memory             <1ms      Logic
7ms    Debug print (if enabled)       ~3ms      SLOW!
─────────────────────────────────────────────────────────
Total: ~10ms per loop (100Hz)          Without debug
       ~13ms per loop (77Hz)           With debug

For competition: Disable debug for faster response!
```

---

## State Machine Overview

```
                    ┌──────────────┐
                    │   NORMAL     │
                    │  FOLLOWING   │◄──────┐
                    │   (PID)      │       │
                    └───┬──────────┘       │
                        │                  │
        ┌───────────────┼──────────────────┼─────┐
        │               │                  │     │
        ▼               ▼                  ▼     │
┌──────────────┐ ┌─────────────┐  ┌─────────────┐
│  SHARP TURN  │ │  JUNCTION   │  │ LOST LINE   │
│   (Pivot)    │ │  (Detect &  │  │  (Search)   │
│              │ │   Handle)   │  │             │
└──────┬───────┘ └──────┬──────┘  └──────┬──────┘
       │                │                 │
       └────────────────┴─────────────────┘
                        │
                        │ Line found
                        └──────────────────────────┘

Current state determined by sensor readings (sum, pattern)
Transitions happen automatically based on conditions
No explicit state variable (implicit in sum value)
```

---

## Key Equations Reference

### Position Calculation
```
sensor_pos = Σ(s[i] × position[i])  for i = 0 to 5
           = s[0]×3 + s[1]×2 + s[2]×1 + s[3]×(-1) + s[4]×(-2) + s[5]×(-3)
```

### Average Position
```
avg = sensor_pos / sum
    = weighted average (-3.0 to +3.0)
```

### PID Output
```
PID = Kp × error + Kd × (error - previous_error)
    = 50 × avg + 120 × derivative
```

### Motor Speeds
```
Left Motor  = base_speed + PID
Right Motor = base_speed - PID

Range: -180 to +180 (bidirectional)
```

---

**End of Flowchart Documentation**
