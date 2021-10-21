# basic avr makefile
# bits from here and there, mostly inspired from Peter Fleury makefiles IIRC

TARGET=main

MCU=atmega328p
ARCH=-mmcu=$(MCU)
F_CPU=16000000
BAUD=115200
PORT=/dev/ttyUSB0

PROGRAMMER=arduino

CC=avr-gcc
OBJCOPY=avr-objcopy
AVRSIZE=avr-size
AVRDUDE=avrdude

EXTRAINCDIRS = include
OPT = s

CFLAGS  = -Wall -std=gnu99
CFLAGS +=-DF_CPU=$(F_CPU)UL $(ARCH)
CFLAGS +=-O$(OPT)
CFLAGS += -I. $(patsubst %,-I%,$(EXTRAINCDIRS))

# Minimalistic printf version
PRINTF_LIB_MIN = -Wl,-u,vfprintf -lprintf_min
# Floating point printf version (requires MATH_LIB = -lm below)
PRINTF_LIB_FLOAT = -Wl,-u,vfprintf -lprintf_flt
# If this is left blank, then it will use the Standard printf version.
# PRINTF_LIB =
# PRINTF_LIB = $(PRINTF_LIB_MIN)
PRINTF_LIB = $(PRINTF_LIB_FLOAT)
# MATH_LIB =
MATH_LIB = -lm

LDFLAGS = -Wl,-Map=$(TARGET).map,--cref
LDFLAGS += $(PRINTF_LIB_FLOAT) $(MATH_LIB)

SOURCES=$(TARGET).c src/uart.c src/twimaster.c src/bme280.c
OBJECTS=$(SOURCES:.c=.o)

.PHONY: all clean flash upload debug monitor kill-monitor

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -R .eeprom -O ihex $< $@

$(TARGET).elf: $(OBJECTS)
	$(CC) $(ARCH) $(LDFLAGS) $^ -o $@

%.o: %.c Makefile
	$(CC) $(CFLAGS) $(ARCH) -c -o $@ $<

size: $(TARGET).elf
	$(AVRSIZE) --mcu=$(MCU) --format=avr $(TARGET).elf

flash: $(TARGET).hex
	$(AVRDUDE) -V -c $(PROGRAMMER) -p $(MCU) -D -P $(PORT) -b $(BAUD) -U flash:w:$<:i

kill-monitor:
	screen -ls | \
	grep avr >/dev/null 2>&1; \
	if [ $$? -eq 0 ] ; then screen -X -S avrmonitor quit; fi

monitor:
	gnome-terminal -e "screen -S avrmonitor $(PORT) 57600"

upload: kill-monitor flash size

all: kill-monitor $(TARGET).hex

debug:
	@echo
	@echo "Source files:"   $(SOURCES)
	@echo "MCU, F_CPU, BAUD:"  $(MCU), $(F_CPU), $(BAUD)
	@echo

clean:
	rm -f $(TARGET).elf $(TARGET).hex $(TARGET).obj $(TARGET).map $(SOURCES:.c=.o)
