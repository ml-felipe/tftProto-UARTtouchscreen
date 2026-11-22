CC ?= cc
# Use the audited SPI driver checkout; no source copying or archive extraction.
SPI_DIR ?= ../tftProto_SPIdriver
APP = tftDriverExpansion
APPFLAGS = -D_DEFAULT_SOURCE -DTFT_CENTER_IMAGES -I$(APP)/include -I$(SPI_DIR) -I$(SPI_DIR)/include
CFLAGS ?= -O2 -g
WARNFLAGS = -std=c11 -Wall -Wextra -Wpedantic
SRC = $(APP)/main.c $(APP)/uart_touchscreen.c $(APP)/touch_parser.c $(APP)/calibration.c $(SPI_DIR)/config_and_spi.c $(SPI_DIR)/print.c $(SPI_DIR)/bmp.c
HEADERS = $(wildcard $(APP)/include/*.h $(SPI_DIR)/include/*.h)
all: $(APP)/tftDriver.exe
$(APP)/tftDriver.exe: $(SRC) $(HEADERS)
	$(CC) $(CPPFLAGS) $(APPFLAGS) $(CFLAGS) $(WARNFLAGS) $(LDFLAGS) $(SRC) -lbcm2835 -lm -o $@
TESTFLAGS = -std=c11 -g -Wall -Wextra -Wpedantic -Werror $(SANITIZE)
tests/test_touch: tests/test_touch.c $(APP)/touch_parser.c $(APP)/calibration.c $(APP)/uart_touchscreen.c $(HEADERS)
	$(CC) $(CPPFLAGS) $(APPFLAGS) $(TESTFLAGS) -I$(SPI_DIR)/tests $(filter %.c,$^) -lm -o $@
tests/test_uart: tests/test_uart.c $(APP)/touch_parser.c $(APP)/uart_touchscreen.c $(HEADERS)
	$(CC) $(CPPFLAGS) $(APPFLAGS) $(TESTFLAGS) -DSERIAL_PORT=\"tests/serial-test\" $(filter %.c,$^) -o $@
tests/test_calibration_ui: tests/test_calibration_ui.c $(APP)/calibration.c $(HEADERS)
	$(CC) $(CPPFLAGS) $(APPFLAGS) $(TESTFLAGS) -I$(SPI_DIR)/tests $(filter %.c,$^) -lm -o $@
tests/test_images: tests/test_images.c $(SPI_DIR)/print.c $(SPI_DIR)/bmp.c $(SPI_DIR)/config_and_spi.c $(SPI_DIR)/tests/mock_bcm2835.c $(HEADERS)
	$(CC) $(CPPFLAGS) $(APPFLAGS) $(TESTFLAGS) -I$(SPI_DIR)/tests $(filter %.c,$^) -o $@
test: tests/test_touch tests/test_uart tests/test_calibration_ui tests/test_images
	./tests/test_touch
	./tests/test_uart missing
	./tests/test_uart
	./tests/test_calibration_ui
	cd $(APP) && ../tests/test_images
host-build:
	$(CC) $(CPPFLAGS) $(APPFLAGS) $(TESTFLAGS) -I$(SPI_DIR)/tests $(SRC) $(SPI_DIR)/tests/mock_bcm2835.c -lm -o tests/demo_host
analyze:
	@set -e; for source in $(SRC); do $(CC) $(CPPFLAGS) $(APPFLAGS) $(TESTFLAGS) -fanalyzer -I$(SPI_DIR)/tests -c $$source -o /dev/null; done
clean:
	rm -f $(APP)/tftDriver.exe tests/test_touch tests/test_uart tests/test_calibration_ui tests/test_images tests/demo_host
.PHONY: all test host-build analyze clean
