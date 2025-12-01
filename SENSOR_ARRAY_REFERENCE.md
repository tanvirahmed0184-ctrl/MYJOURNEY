# Sensor Array Reference Guide

Quick visual reference for your 6-sensor TCRT array configuration.

---

## Physical Layout

### Top View of Robot

```
        FRONT OF ROBOT
        ===============
              ↑
              |
              | direction of movement
              |

    [s5] [s4] [s3] [s2] [s1] [s0]
     A5   A4   A3   A2   A1   A0    ← Arduino pins
     
     └─────────────────────────┘
            Sensor Array
            
    LEFT                    RIGHT
    
        ███████████████               ← black line
```

### Key Points:
- **s[0]** = Rightmost sensor = **A0** = Detects **right** sharp turns
- **s[5]** = Leftmost sensor = **A5** = Detects **left** sharp turns
- **s[2], s[3]** = Middle sensors = Line following
- **Array spans**: ~6-8cm typically
- **Sensor spacing**: ~10-15mm apart

---

## Sensor Positions & Weights

```cpp
int position[6] = {3, 2, 1, -1, -2, -3};
                  s5 s4 s3  s2  s1  s0
```

### Why These Values?

```
    [s5]  [s4]  [s3]  [s2]  [s1]  [s0]
     +3    +2    +1    -1    -2    -3
     
    LEFT         CENTER         RIGHT
    
    When line is to the LEFT:
    - Leftmost sensors (s5,s4,s3) activate
    - Position sum is POSITIVE
    - avg > 0 → PID positive → left motor speeds up
    - Robot turns LEFT ✓
    
    When line is to the RIGHT:
    - Rightmost sensors (s0,s1,s2) activate
    - Position sum is NEGATIVE
    - avg < 0 → PID negative → right motor speeds up
    - Robot turns RIGHT ✓
    
    When line is CENTERED:
    - Middle sensors (s2,s3) activate
    - Position sum ≈ 0
    - avg ≈ 0 → PID ≈ 0 → both motors equal
    - Robot goes STRAIGHT ✓
```

---

## Sensor Reading Examples

### Example 1: Line Centered
```
[s5] [s4] [s3] [s2] [s1] [s0]
  0    0    1    1    0    0

      ████████████            ← black line centered
      
sensor_pos = (0×3) + (0×2) + (1×1) + (1×-1) + (0×-2) + (0×-3)
           = 0 + 0 + 1 + (-1) + 0 + 0
           = 0
           
sum = 2

avg = sensor_pos / sum = 0 / 2 = 0
PID = kp × 0 + kd × derivative = 0

Left motor = lbase + PID = 120 + 0 = 120
Right motor = rbase - PID = 120 - 0 = 120

Result: STRAIGHT ✓
```

---

### Example 2: Line to the Right
```
[s5] [s4] [s3] [s2] [s1] [s0]
  0    0    0    1    1    0

             ████████████      ← black line right of center
             
sensor_pos = (0×3) + (0×2) + (0×1) + (1×-1) + (1×-2) + (0×-3)
           = 0 + 0 + 0 + (-1) + (-2) + 0
           = -3
           
sum = 2

avg = sensor_pos / sum = -3 / 2 = -1.5
PID = kp × -1.5 = 40 × -1.5 = -60

Left motor = lbase + PID = 120 + (-60) = 60
Right motor = rbase - PID = 120 - (-60) = 180

Result: RIGHT motor faster → robot turns RIGHT ✓
```

---

### Example 3: Line to the Left
```
[s5] [s4] [s3] [s2] [s1] [s0]
  0    1    1    0    0    0

  ████████████                 ← black line left of center
  
sensor_pos = (0×3) + (1×2) + (1×1) + (0×-1) + (0×-2) + (0×-3)
           = 0 + 2 + 1 + 0 + 0 + 0
           = 3
           
sum = 2

avg = sensor_pos / sum = 3 / 2 = 1.5
PID = kp × 1.5 = 40 × 1.5 = 60

Left motor = lbase + PID = 120 + 60 = 180
Right motor = rbase - PID = 120 - 60 = 60

Result: LEFT motor faster → robot turns LEFT ✓
```

---

### Example 4: Sharp Right Turn (only s[0])
```
[s5] [s4] [s3] [s2] [s1] [s0]
  0    0    0    0    0    1

                    ████       ← sharp right turn
                    ████
                    ████
                    
This triggers:
if(s[0] && !s[1] && !s[2] && !s[3] && !s[4] && !s[5])

Action:
1. Move forward sharp_turn_forward_time ms
2. Execute do_sharp_turn_right()
3. Spin until middle sensors find line
```

---

### Example 5: Sharp Left Turn (only s[5])
```
[s5] [s4] [s3] [s2] [s1] [s0]
  1    0    0    0    0    0

  ████                         ← sharp left turn
  ████
  ████
  
This triggers:
if(s[5] && !s[4] && !s[3] && !s[2] && !s[1] && !s[0])

Action:
1. Move forward sharp_turn_forward_time ms
2. Execute do_sharp_turn_left()
3. Spin until middle sensors find line
```

---

### Example 6: T-Section (all sensors)
```
[s5] [s4] [s3] [s2] [s1] [s0]
  1    1    1    1    1    1

  ████████████████████████     ← horizontal line (T-junction)
          ████
          ████                 ← robot approaching from bottom
          
sum = 6 (all sensors active)

This triggers:
if(sum == 6) → wait → read again → if(sum >= 3) → T-section

Action:
- Stop
- Confirm junction (delay 150ms)
- Alternate turn (left or right)
```

---

### Example 7: Dead-End (all black → all white)
```
[s5] [s4] [s3] [s2] [s1] [s0]
  1    1    1    1    1    1     ← at dead-end wall

  ████████████████████████      ← solid black wall
          ████
          ████
          
(after moving forward 150ms)

[s5] [s4] [s3] [s2] [s1] [s0]
  0    0    0    0    0    0     ← now all white

sum = 6 → delay → sum = 0

This triggers:
if(sum == 6) → wait → read again → if(sum == 0) → dead-end

Action:
- Execute U-turn (180° spin)
- Find line again
- Continue in opposite direction
```

---

## Sensor Calibration Values

### Typical Analog Values

**White Surface:**
- Good sensors: 50-250
- Marginal: 250-400
- Poor: 400-500

**Black Line:**
- Good sensors: 700-950
- Marginal: 600-700
- Poor: 500-600

**Threshold Calculation:**
```
threshold = (white_max + black_min) / 2

Example:
- White readings: 100-200
- Black readings: 800-900
- Threshold = (200 + 800) / 2 = 500 ✓

Good contrast:
- White = 100, Black = 900
- Difference = 800 (excellent!)

Poor contrast:
- White = 400, Black = 600
- Difference = 200 (might not work well)
```

---

## Physical Installation Guide

### Sensor Height Above Ground

```
Side View:

    [SENSOR ARRAY]
         |
         | ← 2-5mm gap (optimal)
         |
    ════════════════  ground / line
    
Too high (>8mm):
- Weak signal
- Poor black/white distinction
- Unreliable detection

Too low (<1mm):
- Risk of scratching sensors
- Might not work on rough surfaces

Optimal: 2-5mm
- Strong signal
- Good contrast
- Reliable detection
```

### Sensor Alignment

```
Top View:

CORRECT:
    [s5][s4][s3][s2][s1][s0]
    ═══════════════════════   ← perpendicular to line
           ████

INCORRECT:
    [s5][s4][s3][s2][s1][s0]
      ═══════════════════════ ← angled (will cause drift)
            ████
```

**Critical:** Sensor array must be **perpendicular** to the line direction!

---

## Troubleshooting Sensor Issues

### Problem: Robot drifts to one side

**Check:**
1. Are sensors aligned perpendicular to line?
2. Are all sensors at same height?
3. Is one sensor dirty or damaged?
4. Are motor speeds balanced? (lbase vs rbase)

**Test:** Place robot on line, check serial monitor:
```
S: 0 0 1 1 0 0   ← Good (middle sensors)
S: 0 0 1 0 0 0   ← Bad (only one middle sensor)
S: 1 1 1 1 0 0   ← Bad (too many left sensors)
```

---

### Problem: Sensors don't detect line

**Check:**
1. Sensor height (2-5mm is optimal)
2. Threshold value (recalibrate!)
3. Sensor connections (A0-A5)
4. Line color vs background (need high contrast)
5. Lighting conditions (avoid direct sunlight)

**Test:** Upload sensor test code, check raw values:
```cpp
void loop(){
  for(int i=0; i<6; i++){
    Serial.print(analogRead(i)); 
    Serial.print(" ");
  }
  Serial.println();
  delay(500);
}
```

Expected:
- White: 50-300
- Black: 700-950
- If values are similar (e.g., all 400-600), sensors aren't working properly

---

### Problem: Sharp turns not detected

**Symptom:** Robot goes straight off 90° turns

**Check:**
1. Is edge sensor (s0 or s5) detecting turn?
   - Watch serial monitor: should see `S: 1 0 0 0 0 0` for right turn
2. Is only edge sensor active? (not multiple sensors)
3. Is sharp turn code being executed?
   - Should see "SHARP RIGHT" or "SHARP LEFT" in serial
4. Is line width appropriate for sensor spacing?

**Test:** Place robot at 90° turn, check serial:
```
Expected at right turn:
S: 1 0 0 0 0 0 | SHARP RIGHT

If you see:
S: 1 1 0 0 0 0 | (normal PID continues)
→ Line is too wide or robot not centered on turn
```

---

### Problem: False junction detection

**Symptom:** Robot stops and turns when it shouldn't

**Check:**
1. Is threshold too low? (all sensors activating on normal line)
2. Is line too wide? (covers all 6 sensors even on straight)
3. Is junction confirm delay too short? (false positives)

**Test:** On straight line, check serial:
```
Should see:
S: 0 0 1 1 0 0 | sum=2   ← Normal

If you see:
S: 1 1 1 1 1 1 | sum=6   ← False junction!
→ Line is too wide or threshold wrong
```

**Fix:**
- Increase threshold (make detection less sensitive)
- Narrow the line
- Increase junction confirm delay

---

## Quick Reference

| Sensor | Pin | Position | Weight | Detects |
|--------|-----|----------|--------|---------|
| s[0] | A0 | Rightmost | -3 | Right sharp turns |
| s[1] | A1 | Right-center | -2 | Right corrections |
| s[2] | A2 | Right-middle | -1 | Center-right line |
| s[3] | A3 | Left-middle | +1 | Center-left line |
| s[4] | A4 | Left-center | +2 | Left corrections |
| s[5] | A5 | Leftmost | +3 | Left sharp turns |

### Detection Patterns

| Pattern | Meaning | Action |
|---------|---------|--------|
| `0 0 1 1 0 0` | Centered | Straight |
| `0 0 0 1 1 0` | Right of center | Turn right (PID) |
| `0 1 1 0 0 0` | Left of center | Turn left (PID) |
| `1 0 0 0 0 0` | Sharp left | Execute sharp left turn |
| `0 0 0 0 0 1` | Sharp right | Execute sharp right turn |
| `1 1 1 1 1 1` | Junction | Stop, confirm, turn |
| `0 0 0 0 0 0` | Lost line | Continue last direction |

---

**Print this page and keep it handy while testing your robot! 📋**
