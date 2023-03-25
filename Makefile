TARGET     = power-meter
C_FILES    = include/time.c include/oled.c include/i2c.c
C_FILES   += include/ina219.c include/buzzer.c include/encoder.c
C_FILES   += meter.c main.c
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

# ROM: Non-volatile memory ROM that can be programmed for many times, with the capacity of 16KB,
# can all be used for program storage. Or it can be divided into a 14KB program storage area and
# a 2KB BootLoader/ISP program area.
# RAM: 256-byte internal iRAM, which can be used for fast temporary storage of data and stack.
# 1KB on-chip xRAM, which can be used for temporary storage of large amount of data and direct
# memory access (DMA).
# The internal data address space, with 256 bytes in total, as shown in Fugure 6.1, has been all
# used for SFR and iRAM, in which iRAM is used for stack and fast temporary data storage, and can
# be subdivided into the working registers R0-R7, bit variable bdata, byte variable data and idata,
# etc.
# - Flash:        [0000H, 37FFH] - 14k / 14336 bytes
# - Internal RAM: [  00H,   7FH] - 128 bytes (Registers R0 ~ R7 take the first 8 bytes)
# - External RAM: [0000H, 03FFH] - 1k
size:
	@echo "---------------------------------"
	@echo "Summary"
	@echo "---------------------------------"
	@echo "FLASH:        $(shell awk '$$1 == "ROM/EPROM/FLASH"      {printf "%5d", $$4}' $(TARGET).mem) / 14336 bytes"
	@echo "Internal RAM: $(shell awk '$$1 == "Stack"           {printf "%5d", 248-$$10}' $(TARGET).mem) /   120 bytes"
	@echo "External RAM: $(shell awk '$$1 == "EXTERNAL" {printf "%5d", $(XRAM_LOC)+$$5}' $(TARGET).mem) /  1024 bytes"
	@echo "---------------------------------"

clean:
	rm -f *.asm *.lst *.mem *.rel *.rst *.sym *.adb *.lk *.map *.mem *.ihx *.hex *.bin
