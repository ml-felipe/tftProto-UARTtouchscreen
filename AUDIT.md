# Source audit and verification

Audit date: 2026-11-22. Starting commit: `a73cd28`. Scope: Pi sources/headers, PIC firmware/configuration, MPLAB project/build files, README, and source copies inside `newTftDriver.tar.gz`. Recordings, archive and historical build artifacts remain unchanged. The archive has a centered-image renderer and lacks the later freehand coordinate edit; it is explicitly documented as historical.

## Verified issues addressed

| Area | Finding and change |
| --- | --- |
| Linux UART | Initialization continued after open failure and ignored termios errors; VMIN/VTIME were unspecified. Setup now returns errors, closes failed descriptors and explicitly configures blocking 9600 8N1. Close is idempotent. |
| Serial frames | Returning the first parsed frame discarded the rest of that read; parser state was local. Buffered bytes and parser state now persist. The parser checks 10-bit ranges, header, footer and the existing checksum. Disconnects return an error instead of looping forever. |
| Time | Wall-clock timestamps and `long` milliseconds could jump or overflow on 32-bit Pi systems. Touch-gap timing now uses monotonic time and 64-bit milliseconds. |
| Headers/build | Headers defined shared objects in every translation unit, causing duplicate definitions with modern compilers. Globals have single definitions and `extern` declarations. A root Makefile links the sibling SPI source and libm, without copying source from an archive. |
| Calibration retry | Retrying target zero decremented its index to -1 before erasing it. The target is now erased before advancing, and a regression test repeats the first point. EOF and UART errors reset calibration state. |
| Calibration math | Unused allocated matrices were written without allocation checks; singular fits terminated the application. The existing normal-equation solver is retained without those allocations, checks inputs/conditioning, returns failure and publishes only a successful fit. |
| Coordinates | Unsigned coordinates lost negative/fractional affine results. `Pair` uses doubles; freehand coordinates are checked before integer conversion. Removed the extra scaling of already pixel-valued coordinates. Modes require a successful calibration. |
| GUI | Bitwise complement produced -1/0 instead of logical GPIO levels. It now toggles 0/1 on a newly detected press. The legacy centered image placement is preserved through the sibling driver build option. |
| PIC startup | ADC interrupts could transmit before UART initialization. Sampling interrupts start after UART enable. Added forward declarations and service of the configured watchdog in the idle loop. |
| Menu | EOF and invalid/overflowing numeric input now exit instead of hanging or reusing stale values. SPI/UART startup failures release initialized resources. |

## Checks actually run

Linux host: GCC 16.2.1 and Clang 23.1.4, with the companion SPI audit branch:

- `make test host-build analyze`: passed with GCC. Host targets treat warnings as errors; GCC `-fanalyzer` compiled each Pi application source. The host application links a mock SPI backend.
- Clean Clang builds and tests with AddressSanitizer/UndefinedBehaviorSanitizer: passed with `ASAN_OPTIONS=detect_leaks=0`. LeakSanitizer itself cannot operate under this container's tracing restrictions; no leak-check result is claimed.
- Parser tests: concatenated frames, partial-frame prefixes, malformed headers/footer/checksum/ranges and resynchronization. Low-byte corruption is explicitly excluded from detection expectations because the legacy checksum does not cover it.
- Calibration tests: known affine transform including negative output, singular and non-finite input, insufficient samples, unchanged result on failure, first-target retry and I/O failure cleanup.
- Linux pseudo-terminal tests: missing device, configured baud/8N1/raw input, two frames written together, retained unread data, hangup and repeated close. These are OS-level tests, not physical serial tests.
- `git -c core.whitespace=cr-at-eol diff --check`: passed; PIC source retains its original CRLF line endings.
- Production Pi build: blocked by missing bcm2835 headers/library.
- Forced MPLAB make build: blocked by missing MPLAB utility and XC16 installation. No firmware build, flashing or hardware test passed or ran to completion. Existing binaries are not evidence for the edited firmware.

## Remaining limitations and hardware work

The checksum omits both low coordinate bytes. Changing it requires coordinated protocol changes and was deferred to preserve compatibility. There are no release frames, timestamps or sample sequence numbers; host reception gaps are only a heuristic, especially when data queues up. GUI/freehand loops still need interruption to exit and do not guarantee GPIO reset on signals.

The PIC ISR switches channels while auto-sampling, delays using empty loops, then reads the latest ADC buffer and performs blocking UART writes. Channel settling/conversion association and optimization-sensitive delays require measurement and a separate firmware change; no sampling correctness claim is made. Check UART1 pin configuration, oscillator/baud timing and watchdog behavior on the actual board. The affine solver still uses normal equations and has no residual-quality threshold or persistent calibration storage.

Before merging, build with XC16, verify ADC/channel timing and frame bytes on a logic analyzer, then test calibration retries, release/press recognition, the centered GUI and freehand drawing on the original setup. The display driver's register-width change also needs a bench check. All existing demo media remain available but predate these changes.
