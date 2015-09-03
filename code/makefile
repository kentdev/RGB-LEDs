DEVICE = attiny44a
AVRDUDE_MCU = t44
CLOCK = 10000000
AVRDUDE_PROGRAMMER = avrispmkii
AVRDUDE_PORT = usb

CFLAGS = -Wall -funsigned-bitfields -fshort-enums -std=gnu99 -Os

FILES = RGB_LED_strip.c \
        sendToLED.c

DEFINES = -DF_CPU=$(CLOCK) -DF_CLOCK=$(CLOCK)

COMPILE = avr-gcc $(CFLAGS) $(DEFINES) -mmcu=$(DEVICE)
LINK = $(COMPILE)


# symbolic targets: 
all:	main.hex

install: flash 

# fuse bytes = Clock not prescaled, using external 16MHz clock with 65ms startup
#              time, brown-out detection set to 2.7V
fuse:
	avrdude -c $(AVRDUDE_PROGRAMMER) -p $(AVRDUDE_MCU) -P $(AVRDUDE_PORT) -U lfuse:w:0xff:m -U hfuse:w:0xdd:m -U efuse:w:0x01:m

info: all
	avr-size -C --mcu=$(DEVICE) main.elf

flash: all
	avrdude -c $(AVRDUDE_PROGRAMMER) -p $(AVRDUDE_MCU) -P $(AVRDUDE_PORT) -e -U flash:w:main.hex

clean:
	rm -f main.hex main.elf $(MAIN) $(CHILDREN)

# file targets:
main.elf: $(FILES) $(O2_FILES)
	$(COMPILE) $(FILES) -O2 $(O2_FILES) -o $@

main.hex: main.elf
	rm -f main.hex
	avr-objcopy -j .text -j .data -O ihex main.elf main.hex

# Targets for code debugging and analysis:
disasm:	main.elf
	avr-objdump -d main.elf

cpp:
	$(COMPILE) -E main.c

