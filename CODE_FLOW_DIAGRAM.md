# Line Follower Code Flow Diagram

Visual representation of how the fixed code works.

---

## 🔄 Main Program Flow

```
┌─────────────────────────────────────────┐
│           SETUP()                        │
│  - Initialize pins                       │
│  - Start serial communication            │
└─────────────────┬───────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────┐
│           LOOP()                         │
│  - Call semi_pid()                       │
│  - Repeat forever                        │
└─────────────────┬───────────────────────┘
                  │
                  ▼
         ┌────────────────┐
         │   semi_pid()   │
         └────────┬───────┘
                  │
                  ▼
```

---

## 🧠 semi_pid() Decision Tree

```
                    START
                      │
                      ▼
              ┌───────────────┐
              │  reading()    │ ← Read all 6 sensors
              │  sum sensors  │
              └───────┬───────┘
                      │
         ┌────────────┼────────────┐
         │                         │
         ▼                         ▼
    sum == 6?                  sum != 6
         │                         │
        YES                        │
         │                         │
         ▼                         │
    ┌─────────┐                   │
    │  STOP   │                   │
    │ delay   │                   │
    │ read()  │                   │
    └────┬────┘                   │
         │                         │
    ┌────┴─────┐                  │
    │          │                  │
    ▼          ▼                  │
sum==0?    sum>=3?                │
    │          │                  │
   YES        YES                 │
    │          │                  │
    ▼          ▼                  │
┌────────┐ ┌──────────┐           │
│U-TURN  │ │T-SECTION │           │
│Dead-end│ │Alternate │           │
│(return)│ │  turns   │           │
└────────┘ │(return)  │           │
           └──────────┘           │
                                  │
                ┌─────────────────┘
                │
                ▼
         ┏━━━━━━━━━━━━━━━━━━━━━━━┓
         ┃  SHARP TURN CHECK      ┃
         ┗━━━━━━━━┯━━━━━━━━━━━━━━━┛
                  │
      ┌───────────┼───────────┐
      │                       │
      ▼                       ▼
  s[0] only?              s[5] only?
      │                       │
     YES                     YES
      │                       │
      ▼                       ▼
┌────────────┐         ┌────────────┐
│Move forward│         │Move forward│
│  150ms     │         │  150ms     │
│SHARP RIGHT │         │SHARP LEFT  │
│  (return)  │         │  (return)  │
└────────────┘         └────────────┘
                  │
                  │ (if no sharp turn)
                  │
                  ▼
         ┏━━━━━━━━━━━━━━━━━━━━━━━┓
         ┃  NORMAL LINE FOLLOW    ┃
         ┗━━━━━━━━┯━━━━━━━━━━━━━━━┛
                  │
                  ▼
         ┌─────────────────┐
         │  sum > 0?       │
         └────┬─────────┬──┘
              │         │
             YES       NO
              │         │
              ▼         ▼
         ┌────────┐  ┌──────────┐
         │  PID   │  │Continue  │
         │Calculate  │last speed│
         └────┬───┘  └──────────┘
              │
              ▼
         ┌─────────────┐
         │Motor Ramp   │ ← Smooth accel/decel
         │Adjust speeds│
         └──────┬──────┘
                │
                ▼
         ┌─────────────┐
         │motor(L, R)  │ ← Drive motors
         └──────┬──────┘
                │
                ▼
         ┌─────────────┐
         │   DEBUG     │ ← Serial print
         │   Print     │
         └─────────────┘
                │
                ▼
              RETURN (loop again)
```

---

## 🔍 Sensor Reading Flow

```
     reading()
        │
        ▼
  ┌─────────────────┐
  │ sensor_pos = 0  │
  │    sum = 0      │
  └────────┬────────┘
           │
           ▼
  ┌──────────────────────┐
  │ FOR i = 0 to 5       │
  │                      │
  │ s[i] = analogRead(i) │ ← Read sensor
  │                      │
  │ s[i] = (s[i] > threshold) ? 1 : 0 │ ← Digitize
  │                      │
  │ sensor_pos += s[i] * position[i]  │ ← Weight
  │                      │
  │ sum += s[i]          │ ← Count active
  │                      │
  └──────────┬───────────┘
             │
             ▼
        DONE

Example:
s = [0, 0, 1, 1, 0, 0]
sensor_pos = 0×3 + 0×2 + 1×1 + 1×(-1) + 0×(-2) + 0×(-3)
           = 0
sum = 2
```

---

## 🎮 PID Calculation Flow

```
     Start PID
        │
        ▼
  ┌─────────────────────┐
  │ avg = sensor_pos/sum │ ← Average position
  └─────────┬───────────┘
            │
            ▼
  ┌─────────────────────┐
  │   error[0] = avg    │ ← Current error
  └─────────┬───────────┘
            │
            ▼
  ┌──────────────────────────────┐
  │ derivative = error[0] - error[1] │ ← Change in error
  └─────────┬────────────────────┘
            │
            ▼
  ┌───────────────────────────────┐
  │ PID = kp×error[0] + kd×derivative │
  └─────────┬─────────────────────┘
            │
            ▼
  ┌──────────────────────┐
  │ PID = constrain(PID, │ ← Limit PID
  │     -120, 120)       │
  └─────────┬────────────┘
            │
            ▼
  ┌───────────────────────┐
  │ error[1] = error[0]   │ ← Save for next time
  └─────────┬─────────────┘
            │
            ▼
  ┌────────────────────────────┐
  │ lmotor_target = lbase + PID │
  │ rmotor_target = rbase - PID │
  └─────────┬──────────────────┘
            │
            ▼
        Motor Ramp

Example:
avg = -1.5 (line to right)
error[0] = -1.5
derivative = -1.5 - 0 = -1.5
PID = 40×(-1.5) + 100×(-1.5) = -60 - 150 = -210
PID = constrain(-210, -120, 120) = -120
lmotor_target = 120 + (-120) = 0
rmotor_target = 120 - (-120) = 240 → capped at 180
Result: Right motor faster → turn right ✓
```

---

## 🏃 Motor Ramp Flow

```
     Motor Ramp
        │
   ┌────┴────┐
   │         │
   ▼         ▼
LEFT      RIGHT
MOTOR     MOTOR
   │         │
   │         │
   ▼         ▼
┌──────────────────────┐
│ actual < target?     │
└────┬─────────────┬───┘
     │             │
    YES           NO
     │             │
     ▼             ▼
┌──────────┐  ┌──────────────┐
│actual += │  │ actual > target? │
│ min(rate,│  └────┬────────┬────┘
│ diff)    │       │        │
└──────────┘      YES      NO
                   │        │
                   ▼        ▼
              ┌─────────┐  DONE
              │actual -=│  (no change)
              │min(rate,│
              │  diff)  │
              └─────────┘

Example (deceleration):
actual = 120
target = 80
diff = 120 - 80 = 40
actual > target? YES
actual -= min(12, 40) = 12
actual = 120 - 12 = 108

Next loop:
actual = 108, target = 80
actual -= min(12, 28) = 12
actual = 96

Continues until actual = target
```

---

## 🔄 Junction Detection Logic

```
      sum == 6?
         │
        YES
         │
         ▼
    ┌─────────┐
    │  STOP   │
    └────┬────┘
         │
         ▼
    ┌─────────┐
    │delay 150│ ← Move slightly forward
    └────┬────┘
         │
         ▼
    ┌─────────┐
    │ reading()│ ← Read again
    └────┬────┘
         │
    ┌────┴────┐
    │         │
    ▼         ▼
 sum==0?   sum>=3?
    │         │
   YES       YES
    │         │
    ▼         ▼
┌─────────┐ ┌──────────────┐
│ALL WHITE│ │STILL ON LINE │
│Dead-end │ │  T-section   │
└────┬────┘ └──────┬───────┘
     │             │
     ▼             ▼
┌─────────┐   ┌──────────────┐
│ U-TURN  │   │last_T_turn?  │
│Spin 180°│   └──────┬───────┘
└─────────┘          │
                ┌────┴─────┐
                │          │
                ▼          ▼
             'l' ?      'r' ?
                │          │
                ▼          ▼
         ┌──────────┐ ┌──────────┐
         │Turn LEFT │ │Turn RIGHT│
         │Set flag  │ │Set flag  │
         │to 'r'    │ │to 'l'    │
         └──────────┘ └──────────┘
         
         (Next T will alternate)
```

---

## 🔀 Sharp Turn Detection

```
     reading()
        │
        ▼
  ┌──────────────────────────────┐
  │ s[0]==1 && all others ==0 ?  │ ← Right edge only
  └─────┬──────────────┬─────────┘
        │              │
       YES            NO
        │              │
        ▼              │
   ┌────────────┐      │
   │Move forward│      │
   │  ~150ms    │      │
   └─────┬──────┘      │
         │             │
         ▼             │
   ┌────────────┐      │
   │Spin RIGHT  │      │
   │until line  │      │
   │  found     │      │
   └─────┬──────┘      │
         │             │
         ▼             │
      RETURN           │
                       │
        ┌──────────────┘
        │
        ▼
  ┌──────────────────────────────┐
  │ s[5]==1 && all others ==0 ?  │ ← Left edge only
  └─────┬──────────────┬─────────┘
        │              │
       YES            NO
        │              │
        ▼              │
   ┌────────────┐      │
   │Move forward│      │
   │  ~150ms    │      │
   └─────┬──────┘      │
         │             │
         ▼             │
   ┌────────────┐      │
   │Spin LEFT   │      │
   │until line  │      │
   │  found     │      │
   └─────┬──────┘      │
         │             │
         ▼             │
      RETURN           │
                       │
        ┌──────────────┘
        │
        ▼
   Continue to
   normal PID
```

---

## 🎯 Turn Completion (Sensor Feedback)

```
   do_sharp_turn_right()
          │
          ▼
   ┌──────────────┐
   │motor(100,-100)│ ← Start turning
   └──────┬───────┘
          │
          ▼
   ┌──────────────┐
   │  reading()   │ ← Check sensors
   └──────┬───────┘
          │
          ▼
   ┌──────────────────┐
   │ WHILE sum==0     │ ← Still turning
   │    OR sum==1     │   (no line yet)
   └──────┬───────────┘
          │
          ├─YES→ ┌──────────────┐
          │      │motor(100,-100)│ ← Keep turning
          │      └──────┬───────┘
          │             │
          │             ▼
          │      ┌──────────────┐
          │      │  delay(30)   │
          │      └──────┬───────┘
          │             │
          │             ▼
          │      ┌──────────────┐
          │      │  reading()   │ ← Check again
          │      └──────┬───────┘
          │             │
          └─────────────┘
          │
         NO (line found!)
          │
          ▼
   ┌──────────────┐
   │  motor(0,0)  │ ← Stop
   └──────┬───────┘
          │
          ▼
       RETURN

Advantage: Turns exactly until line is found
           regardless of speed, battery, surface
```

---

## 📊 State Transitions

```
       ┌────────────────┐
       │  LINE FOLLOW   │ ◄──────┐
       │   (normal)     │        │
       └────┬───┬───┬───┘        │
            │   │   │            │
    ┌───────┘   │   └──────┐     │
    │           │          │     │
    ▼           ▼          ▼     │
┌────────┐ ┌────────┐ ┌────────┐│
│SHARP   │ │JUNCTION│ │ LOST   ││
│ TURN   │ │        │ │ LINE   ││
└───┬────┘ └───┬────┘ └───┬────┘│
    │          │          │     │
    │      ┌───┴───┐      │     │
    │      │       │      │     │
    │      ▼       ▼      │     │
    │  ┌────────┐ ┌────────┐   │
    │  │T-SECTION│ │DEAD-END│   │
    │  │(turn)  │ │(U-turn)│   │
    │  └───┬────┘ └───┬────┘   │
    │      │          │         │
    └──────┴──────────┴─────────┘
         All return to LINE FOLLOW
```

---

## 🔧 Motor Control Flow

```
  motor(left_speed, right_speed)
             │
             ▼
      ┌─────────────┐
      │  Cap speeds │ ← constrain to ±pwm_cap
      └──────┬──────┘
             │
      ┌──────┴──────┐
      │             │
      ▼             ▼
  ┌────────┐   ┌────────┐
  │ LEFT   │   │ RIGHT  │
  │ MOTOR  │   │ MOTOR  │
  └───┬────┘   └───┬────┘
      │             │
      ▼             ▼
  speed >= 0?    speed >= 0?
      │             │
   ┌──┴──┐       ┌──┴──┐
   │     │       │     │
  YES   NO      YES   NO
   │     │       │     │
   ▼     ▼       ▼     ▼
┌─────┐┌─────┐┌─────┐┌─────┐
│ FWD ││ BWD ││ FWD ││ BWD │
│ lmf ││ lmb ││ rmf ││ rmb │
│ HIGH││HIGH ││HIGH ││HIGH │
└──┬──┘└──┬──┘└──┬──┘└──┬──┘
   │      │      │      │
   ▼      ▼      ▼      ▼
┌──────────────────────────┐
│ analogWrite(lms, speed)  │ ← Set PWM
│ analogWrite(rms, speed)  │
└──────────────────────────┘

Example: motor(120, -80)
Left: 120 >= 0 → FWD, PWM=120
Right: -80 < 0 → BWD, PWM=80 (absolute value)
Result: Forward-left arc
```

---

## 🧮 Data Flow Summary

```
SENSORS → reading() → sum, sensor_pos, s[0..5]
                            │
                            ├─→ Junction check (sum==6?)
                            │
                            ├─→ Sharp turn check (s[0] or s[5] only?)
                            │
                            └─→ PID calculation
                                     │
                                     ▼
                               PID = kp×error + kd×derivative
                                     │
                                     ▼
                               target speeds
                                     │
                                     ▼
                               Motor ramp
                                     │
                                     ▼
                               motor(L, R)
                                     │
                                     ▼
                               MOTORS DRIVE
```

---

## 🔄 Complete Loop Timing

```
     ┌─────────────────────────────┐
     │     One Loop Cycle          │
     │                             │
     │  reading()      ~1-2ms      │ ← Analog read x6
     │  Junction check ~0.1ms      │
     │  Sharp check    ~0.1ms      │
     │  PID calc       ~0.1ms      │
     │  Motor ramp     ~0.1ms      │
     │  motor()        ~0.1ms      │
     │  Serial print   ~1-2ms      │ (debug only)
     │                             │
     │  Total: ~3-5ms per loop     │
     │  = 200-333 loops/second     │
     └─────────────────────────────┘

Very responsive! PID updates 200+ times per second.

Note: Delays only occur during special maneuvers:
- Junction detect: 150ms pause
- Sharp turn: 150ms forward + turn time
- T-turn: ~350-500ms total
- U-turn: ~600-1000ms total
```

---

## 🎯 Key Differences: Original vs Fixed

### Original Code Flow:
```
reading() → set turn variable (unused!)
         → junction check #1
         → PID with BROKEN ramp
         → junction check #2 (never triggers!)
```

### Fixed Code Flow:
```
reading() → unified junction check
         → sharp turn with ACTION
         → PID with WORKING ramp
         → sensor feedback turns
```

---

## 📝 Summary

The fixed code has a clear, logical flow:

1. **Read sensors** → Get current state
2. **Check junctions** → Handle T-sections and dead-ends
3. **Check sharp turns** → Execute if needed
4. **Calculate PID** → Determine corrections
5. **Ramp motors** → Smooth speed changes
6. **Drive motors** → Execute movement

Each step is independent and complete. No conflicting checks, no unused variables, no broken logic!

**The original code had the right structure but buggy implementation.
The fixed code maintains the structure and fixes all bugs! ✅**

---

*Print this flow diagram for reference while coding or debugging!* 📋
