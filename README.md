# CENG336 — Introduction to Embedded Systems Development

METU Computer Engineering, 2025–2026 Spring. Bare-metal firmware for the
**PIC18F8722** on the department's development board — MPLAB X, XC8, and the
datasheet. No RTOS, no HAL, no dynamic memory.

| | Assignment | What it is |
|---|---|---|
| [THE1](THE1) | Round-robin I/O controller (Assembly) | Three independent state machines sharing one loop, with no timers and no interrupts |
| [THE2](THE2) | Flappy Bird on the LED matrix (C) | An interrupt-driven game on the PORTC–F LED area with button input |
| [THE3](THE3) | Three-port EV charging cabinet (C) | EUSART link to a site supervisor, ADC thermal cap, 7-segment display |

---

## THE1 — Round robin, in assembly

A progress-bar LED port, a binary-counter LED port and a blinking indicator, all
running at different rates, all from one round-robin loop with **no hardware
timers and no interrupts at all**. Timing comes from counting instruction cycles,
so every deadline has to be met by construction. Buttons pause/resume the
progress bar and flip the counter's direction.

[`final_with_specified_bits.s`](THE1/final_with_specified_bits.s)

## THE2 — Flappy Bird

The bird lives in the PORTC–F LED grid; pipes scroll in from the right out of
`SCENE_ARRAY`, and a button interrupt makes it flap.

The interesting constraint is that the board has one 8-bit-wide view of the
world and Timer0 is the only clock. So:

- **Timer0 is a 50 ms system tick.** The ISR does nothing but reload `TMR0H:L`
  with `0x0BDC`, bump counters, raise flags, and return. Every other rhythm in
  the game is derived from counting those ticks — gravity every 15 (750 ms),
  a scroll every 20 (1 s, and retunable at runtime), +3 score every 40 (2 s).
  `main()` is a flag-dispatch loop; everything crossing the ISR boundary is
  `volatile`.
- **Scrolling is a bit shift.** Every scroll step shifts PORTC–F right by one
  and ORs the next `SCENE_ARRAY` column into bit 7, wrapping to the start of the
  map at the end. Column 0 is stashed in `pipe_C0..pipe_F0` first — that's what
  the bird is about to collide with.
- **The bird blinks without eating a pipe.** `draw_bird()` sets its bit;
  `delete_bird()` restores the stashed pipe bit for that row instead of clearing
  it. Collision is then just "is my row's stashed bit set?".
- **Buttons are external interrupts** on RB0/RB1/RB2, not polled — flap, and
  live adjustment of the scroll rate.

[`main.c`](THE2/main.c) · [`implementation-notes.pdf`](THE2/implementation-notes.pdf)
is my function-by-function writeup (TR).

## THE3 — EV charging cabinet

Group assignment. A three-port EV charging cabinet that takes commands from a
Python site supervisor over a serial link, reads a thermal sensor on the ADC, and
authorizes an effective current limit `ee = min(requested_limit, thermal_cap)`
that it reports back in a periodic status frame.

**My part was the communication module** —
[`communicate.c`](THE3/communicate.c) / [`communicate.h`](THE3/communicate.h):

- EUSART at 115200 8N1 on RC6/RC7 (`BRG16=1`, `BRGH=1`, `SPBRG=86`, 0.22 % error).
- Two 64-byte single-producer/single-consumer ring buffers, one per direction,
  lock-free and `volatile`. The ISR only moves a byte; all parsing happens in the
  main loop.
- Silent recovery from receiver errors: toggle `CREN` on `OERR`, discard the byte
  on `FERR`, and *never* acknowledge a frame that came in broken.
- A two-state parser for `$...#` frames, recognising `$GO#`, `$END#`, `$CONp#`,
  `$DISp#`, `$LIMxx#` with payload validation, and silently dropping anything
  malformed.
- Builders for the 13-byte `$STSmxxxxcee#` status frame and the 7-byte `$ACKCC#`,
  with a `free_space()` check so a frame is never started that can't be finished.

[`my-contribution.pdf`](THE3/my-contribution.pdf) maps each item back to the
numbered specification lines it satisfies. The rest of the integrated firmware —
[`main.c`](THE3/main.c) with the lifecycle state machine, tick scheduler and
7-segment paging, and [`adc.c`](THE3/adc.c) with the thermal bands
(`<700` → 24 A, `700–899` → 8 A, `≥900` → 0 A) — is the team's shared work.

---

These folders hold the source files only. MPLAB X project scaffolding
(`nbproject/`, `build/`, `dist/`) and the course-provided student pack are left
out; drop the sources into an XC8 project for the PIC18F8722 to build.
