# UART touchscreen expansion

Undergraduate DSBM project at FIB extending the [TFT SPI driver](https://github.com/ml-felipe/tftProto_SPIdriver) with resistive touchscreen input. A PIC24 samples the panel, sends coordinates over UART, and a Raspberry Pi calibrates and displays the input. Existing demonstrations include calibration, freehand drawing and a GPIO-controlled GUI button.

## Architecture and hardware

- `uart_pic24F_touchscreen.X/`: MPLAB X project targeting **PIC24F16KA101**, with XC16 2.10 recorded in project settings.
- `tftDriverExpansion/uart_touchscreen.c`: Linux serial configuration and buffered reads.
- `touch_parser.c`: incremental frame validation, independent of the serial device.
- `calibration.c`: five-point affine least-squares fit from raw coordinates to display pixels.
- `main.c`: terminal menu, calibration, drawing and GUI demos.
- The sibling SPI driver supplies display initialization, drawing and BMP decoding.

Original hardware: Raspberry Pi 2, TFT Proto rev 1.01 and PIC24F16KA101. Firmware assigns X+ to RA0, X− to RA1, Y+ to RB0 and Y− to RB1. The Pi reads `/dev/serial0` at 9600 baud, 8 data bits, no parity, one stop bit and no flow control. Connect PIC UART1 TX to the Pi UART RX with compatible 3.3 V logic and common ground; verify the PIC package pinout and board wiring. Disable the Linux serial console on that UART and ensure access permissions. The GUI uses BCM GPIO21 (physical pin 40) as its output; use an appropriate external circuit for the load. Display wiring is documented in the SPI repository.

## Serial protocol

Each frame is eight bytes:

| Offset | Content |
| --- | --- |
| 0–1 | ASCII `XY` |
| 2–3 | X, big-endian, 10-bit unsigned ADC value |
| 4–5 | Y, big-endian, 10-bit unsigned ADC value |
| 6 | XOR of bytes 2 and 4 |
| 7 | Newline (`0x0a`) |

The parser retains partial frames and unread bytes across calls and searches again after malformed input. The checksum is intentionally unchanged for firmware compatibility, but **does not protect the coordinate low bytes**. Payload bytes may equal header or newline bytes; this is a fixed-length protocol, not line-oriented text. There is no explicit release event. A 100 ms gap between received frames is used as a release heuristic, so host buffering and scheduling can affect touch continuity.

## Build the Raspberry Pi application

Use sibling checkouts. During review, the SPI fixes are on `fix/embedded-audit`; this expansion depends on that version's `bmp.c`, tests and centered-image option.

```sh
git clone --branch fix/embedded-audit https://github.com/ml-felipe/tftProto_SPIdriver.git
git clone --branch fix/embedded-audit https://github.com/ml-felipe/tftProto-UARTtouchscreen.git
cd tftProto-UARTtouchscreen
make                         # SPI_DIR defaults to ../tftProto_SPIdriver
cd tftDriverExpansion
sudo ./tftDriver.exe
```

Install the bcm2835 library as described in the SPI README. The application also links the standard math library (`-lm`). For a different checkout location, use `make SPI_DIR=/path/to/tftProto_SPIdriver`. The build enables centered image placement to preserve the original GUI archive's behavior. Run from `tftDriverExpansion/` for `on.bmp` and `off.bmp`; other image names can be supplied by copying the existing SPI demo images into that runtime image directory.

The historical `newTftDriver.tar.gz` is retained unchanged for reference. It contains older copies of the driver and does not contain these fixes. Source checkouts and the root Makefile are the maintained build path; manual overlaying is unnecessary.

## Build the PIC firmware

Open `uart_pic24F_touchscreen.X` in MPLAB X, select PIC24F16KA101, install/select XC16 and the PIC24F-KA-KL-KM device pack, then regenerate local tool paths and build. Recorded settings use MPLAB X 6.05, XC16 2.10 and device pack 1.3.49. The generated makefiles contain installation-specific paths, so an untouched checkout may not build elsewhere. After configuration, the command-line entry point is:

```sh
make -C uart_pic24F_touchscreen.X
```

Rebuild before programming. Committed `build/` and `dist/` files are historical artifacts, not current verification results. UART is enabled before ADC interrupts can transmit, and the main loop services the configured watchdog. The baud divisor remains 25 with BRGH=0; confirm the clock and baud rate on the bench using the [PIC24F16KA102 family datasheet](https://ww1.microchip.com/downloads/en/DeviceDoc/39927c.pdf).

## Calibration and demos

Select `c`, press each target, and type `c` to accept or `k` to retry. Keep the five points spatially separated. Invalid or singular fits leave the previous calibration unchanged. Calibration lives in RAM and must be repeated after restarting. Transformed coordinates remain signed floating-point values until checked against display bounds; they are already in pixels.

Select `t` for freehand drawing or `g` for the GUI after calibration. Freehand mode fills rectangles between successive samples, matching the original demonstration; it is not a line-interpolation algorithm. The GUI toggles once on a detected new press inside the central button. These two modes run until interruption or a serial error; they do not offer an in-mode return to the menu. Stop safely with the external load disconnected when appropriate; interruption does not guarantee resetting its GPIO level.

## Host checks and limitations

With the audited SPI checkout present, no bcm2835 installation is needed:

```sh
make test host-build
make analyze                 # GCC -fanalyzer
make clean
make test host-build CC=clang SANITIZE='-fsanitize=address,undefined -fno-omit-frame-pointer'
```

Tests cover parser recovery, affine mapping and degenerate input, first-target calibration retry, serial configuration, consecutive frames and disconnects using a Linux pseudo-terminal. They do not measure touch accuracy, ADC settling, UART electrical timing or SPI behavior. The firmware still uses delay loops and blocking UART transmission inside the ADC ISR; conversion/channel timing needs a hardware review. The affine solver retains normal equations and is not intended for ill-conditioned point sets. See [AUDIT.md](AUDIT.md) for results and remaining bench checks.

## Demos

**Calibration**

https://github.com/user-attachments/assets/9cf8e5e5-00c5-408a-8233-3f53516f953b

**Freehand Mode**

https://github.com/user-attachments/assets/f7631ceb-60e0-4256-99d3-4c5e1f2b5abd

**GUI**

https://github.com/user-attachments/assets/c7ddd30c-6a44-4a06-9635-87f4f4609aea

Additional recording: [input detection](tftDriverExpansion/demos/input_detection.mp4). The existing recordings in `tftDriverExpansion/demos/` are retained; they are not tests of the current changes. Original authorship and vendor notices remain in the source.
