TARGET := ~/opt/cross/bin/i686-elf
TARGET_NAME := pastel

CC := $(TARGET)-gcc
AS := nasm
LD := $(TARGET)-gcc

C_FLAGS := -ffreestanding -Wall -Wextra -c
AS_FLAGS := -felf32
LD_FLAGS := -ffreestanding -T linker.ld -nostdlib -lgcc
QEMU_FLAGS := -m 100M -net none $(QEMU_FLAGS)

REL_C_FLAGS := $(C_FLAGS) -O2
REL_AS_FLAGS := $(AS_FLAGS)
REL_LD_FLAGS := $(LD_FLAGS) -O2
REL_QEMU_FLAGS := $(QEMU_FLAGS)

DBG_TARGET_NAME := $(TARGET_NAME)_dbg
DBG_C_FLAGS := $(C_FLAGS) -O0 -g
DBG_AS_FLAGS := $(AS_FLAGS)
DBG_LD_FLAGS := $(LD_FLAGS) -O0 -g
DBG_QEMU_FLAGS := $(QEMU_FLAGS) -s -S

SRC_DIR := src
BUILD_DIR := build
DBG_BUILD_DIR := dbg_build

C_SOURCES := $(shell find $(SRC_DIR) -name '*.c')
ASM_SOURCES := $(shell find $(SRC_DIR) -name '*.asm')

C_OBJECTS := $(addsuffix .o,$(subst $(SRC_DIR)/,$(BUILD_DIR)/,$(C_SOURCES)))
ASM_OBJECTS := $(addsuffix .o,$(subst $(SRC_DIR)/,$(BUILD_DIR)/,$(ASM_SOURCES)))

DBG_C_OBJECTS := $(subst $(BUILD_DIR)/,$(DBG_BUILD_DIR)/,$(C_OBJECTS))
DBG_ASM_OBJECTS := $(subst $(BUILD_DIR)/,$(DBG_BUILD_DIR)/,$(ASM_OBJECTS))

PROGRAMS := $(wildcard user/*/.)
COMMA := ,
INITRDS = -initrd $(subst $(eval ) ,$(COMMA),$(wildcard user/*/*.bin))

.PHONY: clean

all: build

$(C_OBJECTS): build/%.c.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) -o $@ $< $(REL_C_FLAGS)

$(ASM_OBJECTS): build/%.asm.o: src/%.asm
	@mkdir -p $(dir $@)
	$(AS) -o $@ $< $(REL_AS_FLAGS)

$(TARGET_NAME).bin: $(C_OBJECTS) $(ASM_OBJECTS) linker.ld
	$(LD) -o $(TARGET_NAME).bin $(C_OBJECTS) $(ASM_OBJECTS) $(REL_LD_FLAGS)

$(DBG_C_OBJECTS): dbg_build/%.c.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) -o $@ $< $(DBG_C_FLAGS)

$(DBG_ASM_OBJECTS): dbg_build/%.asm.o: src/%.asm
	@mkdir -p $(dir $@)
	$(AS) -o $@ $< $(DBG_AS_FLAGS)

$(DBG_TARGET_NAME).bin: $(DBG_C_OBJECTS) $(DBG_ASM_OBJECTS) linker.ld
	$(LD) -o $(DBG_TARGET_NAME).bin $(DBG_C_OBJECTS) $(DBG_ASM_OBJECTS) $(LD_FLAGS)

programs:
	@$(MAKE) -C user all

build: $(TARGET_NAME).bin programs
dbg_build: $(DBG_TARGET_NAME).bin programs

iso: build grub.cfg
	mkdir -p iso/boot/grub
	cp $(TARGET_NAME).bin iso/boot/$(TARGET_NAME).bin
	cp grub.cfg iso/boot/grub/grub.cfg
	grub-mkrescue -o $(TARGET_NAME).iso iso

dbg_iso: dbg_build grub.cfg
	@rm -r iso 2> /dev/null || true
	mkdir -p iso/boot/grub
	cp $(DBG_TARGET_NAME).bin iso/boot/$(TARGET_NAME).bin
	cp grub.cfg iso/boot/grub/grub.cfg
	grub-mkrescue -o $(TARGET_NAME).iso iso

run: build
	qemu-system-i386 -kernel $(TARGET_NAME).bin $(QEMU_FLAGS) $(INITRDS)

runiso: iso
	qemu-system-i386 -cdrom $(TARGET_NAME).iso $(QEMU_FLAGS)

dbg_run: dbg_build
	qemu-system-i386 -kernel $(DBG_TARGET_NAME).bin $(QEMU_FLAGS) $(INITRDS)

debug: dbg_build
	qemu-system-i386 -kernel $(DBG_TARGET_NAME).bin $(DBG_QEMU_FLAGS) $(INITRDS)

debugiso: dbg_iso
	qemu-system-i386 -cdrom $(TARGET_NAME).iso $(DBG_QEMU_FLAGS)

# I just think it's neat.
runc: build
	@cp ~/.config/kitty/current-theme.conf .
	@sed 's/background_opacity.*/background_opacity 1/' ~/.config/kitty/kitty.conf > kitty.conf
	@echo 'confirm_os_window_close 0' >> kitty.conf
	@kitten themes --reload-in=none --config-file-name=./kitty.conf Catppuccin-Frappe
	kitty --detach --hold --title PastelOS --config ./kitty.conf qemu-system-i386 -kernel $(TARGET_NAME).bin -display curses

clean:
	rm -r $(BUILD_DIR) $(DBG_BUILD_DIR) $(TARGET_NAME).bin $(DBG_TARGET_NAME).bin iso/ $(TARGET_NAME).iso 2> /dev/null || true
	@make -C user clean
