# PIR Motion Alarm — STM32F030RCT6

A bare-metal motion alarm. A PIR sensor arms/triggers the alarm, an active
buzzer sounds it, an LED shows the current state, and a push button disarms it.

Built with STM32CubeMX (HAL) + CMake + Ninja, developed in VS Code with the
STM32Cube extensions.

## Hardware

| Part             | MCU pin | Notes                                                        |
| ---------------- | ------- | ------------------------------------------------------------ |
| PIR sensor (5 V) | PA1     | Sensor powered from 5 V; its OUT pin (3.3 V logic) goes to PA1. Internal pull-down. |
| Push button      | PC8     | Button between PC8 and 3.3 V. Internal pull-down, reads high when pressed. |
| Active buzzer    | PA6     | Push-pull output, high = sounding. Use a transistor driver if the buzzer draws more than a GPIO pin can source. |
| Status LED       | PB8     | Push-pull output, high = lit.                                |

All grounds (MCU, PIR, buzzer) must be common.

## Behaviour

| State        | LED                  | Buzzer              | Leaves when…                                                  |
| ------------ | -------------------- | ------------------- | ------------------------------------------------------------- |
| **WARMUP**   | solid                | off                 | `WARM_UP_TIME_MS` has passed → ARMED (button ignored)         |
| **ARMED**    | slow blink (500 ms)  | off                 | PIR reports motion → ALARM (button ignored)                   |
| **ALARM**    | fast blink (100 ms)  | beeping 200/200 ms  | button pressed → DISARMED; otherwise motion ends → ARMED      |
| **DISARMED** | off                  | off                 | button pressed (early re-arm) or `DISARM_TIME_MS` passed → ARMED |

```
         warm-up done             motion
WARMUP ──────────────► ARMED ─────────────► ALARM
                        ▲  ▲                  │  │
                        │  └──── no motion ───┘  │ button
                        │                        ▼
                        └──── button/timeout ─ DISARMED
```

How long ALARM lasts follows the PIR's own output hold time (the "time"
potentiometer on HC-SR501-style modules).

## Software design

Everything runs from a non-blocking super-loop — no `HAL_Delay()`, every
timing is `HAL_GetTick() - since >= period`, so all modules stay responsive:

```c
while (1) {
  button_update();     // debounce, latch presses
  app_update();        // state machine
  indicator_update();  // LED blink timing
  buzzer_update();     // buzzer on/off timing
}
```

Modules in [Core/Src/pir_app.c](Core/Src/pir_app.c):

- **pir** — reads the PIR output pin.
- **button** — 30 ms debounce; `button_was_pressed()` returns each press once.
- **indicator** — LED patterns `OFF`, `SOLID`, `SLOW_BLINK`, `FAST_BLINK`.
- **buzzer** — patterns `BEEP_OFF`, `BEEP_ALARM`.
- **app** — the state machine. `enter_state()` holds the table of LED/buzzer
  outputs per state; `app_update()` holds the transitions.

System clock: HSI 8 MHz ÷ 2 × 12 (PLL) = **48 MHz**, 1 flash wait state,
SysTick at 1 ms (set in `SystemClock_Config()` in [Core/Src/main.c](Core/Src/main.c)).

### Tuning

All timings and pin assignments are `#define`s in
[Core/Inc/pir_app.h](Core/Inc/pir_app.h):

| Define               | Default | Meaning                                  |
| -------------------- | ------- | ---------------------------------------- |
| `WARM_UP_TIME_MS`    | 3000    | Time before the alarm arms after power-up |
| `DISARM_TIME_MS`     | 3000    | How long DISARMED lasts before auto re-arm |
| `DEBOUNCE_TIME_MS`   | 30      | Button debounce                          |
| `SLOW_BLINK_HALF_MS` | 500     | LED half-period when ARMED               |
| `FAST_BLINK_HALF_MS` | 100     | LED half-period in ALARM                 |
| `BEEP_ON_MS` / `BEEP_OFF_MS` | 200 / 200 | Buzzer on/off times in ALARM     |

Most PIR modules need 30–60 s after power-up to settle; raise
`WARM_UP_TIME_MS` if you see false alarms right after reset.

## Building and flashing

Requirements: VS Code with the **STM32Cube for VS Code** extension pack (it
provides the GNU Arm toolchain, CMake and Ninja).

1. Open the folder in VS Code.
2. Select the **Debug** (or **Release**) CMake preset and build.
   The output is `build/Debug/capstone.elf`.
3. Flash/debug with the **STM32Cube: Launch JLink GDB Server** configuration
   in [.vscode/launch.json](.vscode/launch.json).

From a terminal with the toolchain on `PATH`:

```sh
cmake --preset Debug
cmake --build --preset Debug
```

## Regenerating with CubeMX

User code in `main.c` sits between `USER CODE BEGIN/END` markers and survives
regeneration. **`SystemClock_Config()` does not** — it is rewritten from
`capstone.ioc`. Before regenerating, set the clock in CubeMX to match
(PLL source HSI/2, ×12, SYSCLK 48 MHz), or re-apply the 48 MHz settings
afterwards.

## Project layout

```
Core/Inc/pir_app.h    pins, timings, types, module API
Core/Src/pir_app.c    pir, button, indicator, buzzer, app state machine
Core/Src/main.c       clock setup, init calls, super-loop
capstone.ioc          CubeMX project
cmake/                toolchain files and CubeMX-generated CMake
Drivers/              STM32 HAL and CMSIS (generated)
```
