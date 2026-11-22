# UART touchscreen expansion

Undergraduate DSBM project at FIB extending the [TFT SPI driver](https://github.com/ml-felipe/tftProto_SPIdriver) with resistive touch input. A PIC24 samples the panel and sends coordinates to a Raspberry Pi for calibration, freehand drawing and a GPIO button demonstration.

## Design and hardware

`uart_pic24F_touchscreen.X/` targets PIC24F16KA101. In `tftDriverExpansion/`, `uart_touchscreen.c` configures Linux serial I/O, `touch_parser.c` validates frames, `calibration.c` fits a five-point affine transform, and `main.c` runs the demos. Display code comes from the sibling SPI repository.

Original setup: Raspberry Pi 2, TFT Proto rev 1.01 and PIC24F16KA101. Panel connections in firmware are X+ → RA0, X− → RA1, Y+ → RB0 and Y− → RB1. Connect PIC UART1 TX to Pi UART RX with compatible 3.3 V logic and common ground; verify package pinout and board wiring. Disable the Linux console on `/dev/serial0` and ensure serial-device access. Display wiring is in the SPI README. The GUI output is BCM GPIO21 (header pin 40); use a suitable external load circuit.

## Protocol

UART uses 9600 baud, 8N1, without flow control. Each frame has eight bytes:

| Offset | Value |
| --- | --- |
| 0–1 | ASCII `XY` |
| 2–3 | X: big-endian, 10-bit ADC value |
| 4–5 | Y: big-endian, 10-bit ADC value |
| 6 | XOR of bytes 2 and 4 |
| 7 | `0x0a` |

The parser retains partial frames and unread bytes, rejects malformed frames and resynchronizes. This is a fixed-length binary protocol: payload bytes can contain newline. The legacy checksum does **not** protect the low coordinate bytes. There is no release event; a 100 ms receive gap estimates a new press, so queued data and host scheduling affect continuity.

## Build and use

Install the bcm2835 library as described in the SPI README. During PR review, use both audit branches in sibling directories:

```sh
git clone --branch fix/embedded-audit https://github.com/ml-felipe/tftProto_SPIdriver.git
git clone --branch fix/embedded-audit https://github.com/ml-felipe/tftProto-UARTtouchscreen.git
cd tftProto-UARTtouchscreen
make
cd tftDriverExpansion
sudo ./tftDriver.exe
```

For another driver location, pass `SPI_DIR=/path/to/tftProto_SPIdriver` to Make. The build links libm and enables centered images, preserving the original GUI layout. Run from `tftDriverExpansion/` for `images/on.bmp` and `off.bmp`. Other SPI demo images can be copied into that directory. The retained `newTftDriver.tar.gz` is historical and does not contain the fixes; use the source build above.

Select `c`, press each target, then type `c` to accept or `k` to retry. Failed fits preserve the previous calibration; calibration is held only in RAM. Select `t` for freehand or `g` for the GUI after calibrating. Coordinates are transformed directly to display pixels. Freehand fills rectangles between samples; the GUI toggles on a detected new press inside the button. Both modes run until interruption or serial failure; interruption does not guarantee resetting GPIO21.

For firmware, open `uart_pic24F_touchscreen.X` in MPLAB X, select PIC24F16KA101 and regenerate local tool paths. Recorded settings use MPLAB X 6.05, XC16 2.10 and PIC24F-KA-KL-KM_DFP 1.3.49. Build in the IDE or, after configuration, with `make -C uart_pic24F_touchscreen.X`. Rebuild before flashing: tracked `build/` and `dist/` files are historical outputs, not builds of the edited firmware.

## Tests and limitations

With the sibling SPI checkout present:

```sh
make test host-build
make analyze                 # GCC -fanalyzer
make clean
make test host-build CC=clang SANITIZE='-fsanitize=address,undefined -fno-omit-frame-pointer'
```

Tests cover frame recovery, affine mapping, calibration retry/failure, Linux pseudo-terminal setup and disconnects, and centered GUI image pixels/window bounds through the shared SPI driver. Clean before changing compiler or sanitizer flags.

GCC 16.2.1 tests, mock-SPI application builds and static analysis passed. Clang 23.1.4 AddressSanitizer/UndefinedBehaviorSanitizer tests passed with `ASAN_OPTIONS=detect_leaks=0`; container tracing restrictions prevented LeakSanitizer. Production builds were blocked by missing bcm2835 and MPLAB/XC16. No PIC, Raspberry Pi or physical touchscreen verification ran.

UART now initializes before ADC interrupts transmit, and the idle loop services the enabled watchdog. These firmware changes and the SPI register-width correction need bench verification. The ISR still switches ADC channels during auto-sampling, uses delay loops and blocks on UART transmission: check settling, conversion association, optimization effects and watchdog behavior. Confirm oscillator/baud timing against the [device datasheet](https://ww1.microchip.com/downloads/en/DeviceDoc/39927c.pdf). The affine solver uses normal equations without a residual-quality threshold; calibration accuracy has not been measured.

## Demos

**Calibration**

https://github.com/user-attachments/assets/9cf8e5e5-00c5-408a-8233-3f53516f953b

**Freehand Mode**

https://github.com/user-attachments/assets/f7631ceb-60e0-4256-99d3-4c5e1f2b5abd

**GUI**

https://github.com/user-attachments/assets/c7ddd30c-6a44-4a06-9635-87f4f4609aea

Additional recording: [input detection](tftDriverExpansion/demos/input_detection.mp4). Existing videos predate these fixes; media and source/vendor credits are retained.
