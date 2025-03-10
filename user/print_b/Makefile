TARGET := ~/opt/cross/bin/i686-elf
TARGET_NAME := print_b
CC := $(TARGET)-gcc
AS := nasm
LD := $(TARGET)-gcc

C_FLAGS := -ffreestanding -Wall -Wextra -c -O2
AS_FLAGS := -felf32
LD_FLAGS := -ffreestanding -T ../linker.ld -nostdlib -lgcc

SRC_DIR := src
BUILD_DIR := build

C_SOURCES := $(shell find $(SRC_DIR) -name '*.c')
ASM_SOURCES := $(shell find $(SRC_DIR) -name '*.asm')

C_OBJECTS := $(patsubst %.c,%.c.o,$(subst $(SRC_DIR)/,$(BUILD_DIR)/,$(C_SOURCES)))
ASM_OBJECTS := $(patsubst %.asm,%.asm.o,$(subst $(SRC_DIR)/,$(BUILD_DIR)/,$(ASM_SOURCES)))

.PHONY: clean

all: $(TARGET_NAME).bin

$(C_OBJECTS): build/%.c.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) -o $@ $< $(C_FLAGS)

$(ASM_OBJECTS): build/%.asm.o: src/%.asm
	@mkdir -p $(dir $@)
	$(AS) -o $@ $< $(AS_FLAGS)

$(TARGET_NAME).bin: $(C_OBJECTS) $(ASM_OBJECTS)
	$(LD) -o $(TARGET_NAME).bin $(C_OBJECTS) $(ASM_OBJECTS) $(LD_FLAGS)

clean:
	rm -r $(TARGET_NAME).bin $(BUILD_DIR)/ 2> /dev/null || true
