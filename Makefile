TARGET     = power-meter
C_FILES    = main.c include/time.c include/oled.c include/i2c.c include/ina219.c include/buzzer.c
# ASM_FILES  = bitbang_asm.asm
BUILD_DIR  = build

# MCU Configuration
FREQ_SYS   = 12000000
XRAM_SIZE ?= 0x0400
XRAM_LOC  ?= 0x0000
CODE_SIZE ?= 0x3800

# Toolchain
CC         = sdcc
OBJCOPY    = objcopy
PACK_HEX   = packihx
WCHISP    ?= python3 -m ch55xtool -f
# WCHISP   ?= /opt/homebrew/opt/python@3.10/bin/python3.10 -m ch55xtool -f

# Flags
CFLAGS    := --std-c11 --Werror --opt-code-size -V -mmcs51 --model-small
CFLAGS    += --xram-size $(XRAM_SIZE) --xram-loc $(XRAM_LOC) --code-size $(CODE_SIZE)
CFLAGS    += -Iinclude
CFLAGS    += -DFREQ_SYS=$(FREQ_SYS)
LFLAGS    := $(CFLAGS)
RELS      := $(C_FILES:.c=.rel)

all: $(TARGET).bin $(TARGET).hex size

%.rel : %.c
	$(CC) -c $(CFLAGS) $<

$(TARGET).ihx: $(RELS)
	$(CC) $(notdir $(RELS)) $(LFLAGS) -o $@

$(TARGET).hex: $(TARGET).ihx
	$(PACK_HEX) $< > $@

$(TARGET).bin: $(TARGET).ihx
	$(OBJCOPY) -I ihex -O binary $< $@

pre-flash:

flash: $(TARGET).bin pre-flash
	$(WCHISP) $<

size:
	@echo "------------------"
	@echo "FLASH: $(shell awk '$$1 == "ROM/EPROM/FLASH"      {print $$4}' $(TARGET).mem) bytes"
	@echo "IRAM:  $(shell awk '$$1 == "Stack"           {print 248-$$10}' $(TARGET).mem) bytes"
	@echo "XRAM:  $(shell awk '$$1 == "EXTERNAL" {print $(XRAM_LOC)+$$5}' $(TARGET).mem) bytes"
	@echo "------------------"

clean:
	rm -f *.asm *.lst *.mem *.rel *.rst *.sym *.adb *.lk *.map *.mem *.ihx *.hex *.bin
