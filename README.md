# Linear Peristaltic Dosing Pump

Arduino firmware for a stepper-driven linear peristaltic pump that dispenses a set volume of fluid at a set flow rate. Because the fluid only ever touches the tubing, the pump doses without contamination.

Built as a team project for UVA Biomedical Engineering (Sep – Oct 2026). The pump uses an 18-follower camshaft mechanism with 3D-printed components.

<!-- TODO: add a photo of the pump here, e.g. ![Pump](docs/pump.jpg) -->

## Features

- **Flow rate:** 0.5 to 60 mL/min, adjustable in 0.5 mL/min steps
- **Target volume:** 1 to 500 mL, adjustable in 1 mL steps
- **Automatic stop** when the target volume has been dispensed
- **Non-blocking design:** no `delay()` calls in the main loop, so buttons stay responsive while the motor runs
- **Drift-free step timing** using `micros()`, so the average flow rate stays on target
- **5-button interface** with a 16x2 I2C LCD
- **Hold-to-repeat buttons** that speed up the longer they are held

## Results

Validated over 9 trials at flow rates between 30 and 40 mL/min. Mean volume error was below 4% at all three tested flow rates.

<!-- TODO: add the results table from your testing here -->

## Hardware

| Part | Notes |
|------|-------|
| Microcontroller | Arduino <!-- TODO: board model, e.g. Uno --> |
| Stepper driver | Step/direction driver with active-low enable <!-- TODO: model --> |
| Stepper motor | <!-- TODO: model --> |
| Display | 16x2 LCD with I2C backpack at address `0x27` |
| Buttons | 5 momentary push buttons, wired between the pin and ground |

### Pin assignments

| Pin | Function |
|-----|----------|
| 2 | Stepper STEP |
| 3 | Stepper DIR |
| 4 | Stepper ENABLE (active low) |
| 5 | Start / Stop / Reset button |
| 6 | Rate up |
| 7 | Volume up |
| 8 | Rate down |
| 9 | Volume down |
| SDA / SCL | LCD (I2C) |

All buttons use the Arduino's internal pull-up resistors, so no external resistors are needed.

## Controls

| Action | Result |
|--------|--------|
| Short press Start (while stopped) | Start dispensing. If the last run finished, the count resets first. |
| Press Start (while running) | Stop immediately |
| Hold Start for 1 second (while stopped) | Reset the dispensed volume to zero |
| Press Rate up / down | Change flow rate by 0.5 mL/min |
| Press Volume up / down | Change target volume by 1 mL |
| Hold any adjust button | Auto-repeat after 0.5 s, faster after 3 s |

When stopped, the display shows the rate on the first line and dispensed / target volume on the second line, with a status of `STP` (stopped) or `END` (target reached). While the pump is running, the display shows `Running...`.

## How it works

**Calibration.** The pump was measured to need 850 motor steps per mL (`STEPS_PER_ML`). Everything else is derived from that constant:

```
step interval (µs) = 60,000,000 / (rate × 850)
target steps       = target volume × 850
```

At 60 mL/min this works out to one step about every 1.18 ms.

**Step timing.** The main loop checks `micros()` on every pass and fires a step when the interval has elapsed. The next step time is advanced by exactly one interval instead of being set to the current time, so small delays do not accumulate and the average rate stays accurate. If the loop ever falls more than one interval behind, the timer resyncs instead of firing a burst of catch-up steps.

**Keeping the LCD from disturbing the motor.** I2C writes to the LCD are slow compared to the step interval. To protect step timing, the firmware writes a static `Running...` message once when the pump starts and does not touch the display again until it stops. When stopped, the display is updated one changed character per loop pass, which keeps each pass short.

**Buttons.** Buttons are polled every 20 ms, which also acts as debouncing. The adjust buttons fire once on press, then repeat every 80 ms after being held for 0.5 s and every 20 ms after 3 s, so large changes are quick to dial in.

## Setup

1. Install the Arduino IDE.
2. Install the **LiquidCrystal I2C** library from the Library Manager.
3. Wire the hardware according to the pin table.
4. Open the sketch, select your board and port, and upload.

### Recalibrating

If you change the tubing, cam, or motor, recalibrate:

1. Set a target volume and run the pump into a graduated cylinder or onto a scale.
2. Compute `new STEPS_PER_ML = 850 × (target volume / measured volume)`.
3. Update `STEPS_PER_ML` in the sketch and upload again.

## Team

Developed with my project team at UVA Biomedical Engineering. <!-- TODO: credit teammates if they are comfortable being named -->
