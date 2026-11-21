# Lab 2 - Exercise 10

Reuse the Exercise 9 Proteus circuit. Set the STM32 Program File to `Debug/ex10.hex` (use an absolute path when using the ex9 circuit).

The letter A scrolls circularly to the left: each outgoing left column re-enters at the right. No new wiring is required. Keep the working matrix pull-up/COM supply at 5 V from the verified ex9 simulation; MCU supply remains 3.3 V.

- TIM2: HSI 8 MHz, prescaler 799, period 9 -> 1 ms interrupt.
- Timer 0: 1000 ms, updates the HH:MM clock and colon.
- Timer 1: 5 ms per seven-segment digit.
- Timer 2: 2 ms per matrix column -> 16 ms/frame, approximately 62.5 Hz.
- Timer 3: requests a left shift after 300 ms. The change waits for the next complete frame boundary, so visible shifts occur about every 304 ms.
- The interrupt only updates software timers. Display scanning and scrolling run in main().

Change `MATRIX_SCROLL_MS` in `Core/Src/main.c` to change animation speed. The existing A bitmap is unchanged. `0` bits light pixels on the selected column; `0xFF` is a blank column.

Import this directory as an existing STM32CubeIDE project named ex10. Source functions, variables, and prototypes remain in the corresponding USER CODE blocks. The Debug makefile also supports rebuilding with the STM32 GCC tools on PATH.
