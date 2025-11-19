CC      = clang
TARGET  = main
SOURCES = src/main.c
BUILD   = build

CFLAGS  = -std=c99 -Wall -g -fcolor-diagnostics -fansi-escape-codes -fsanitize=address
CFLAGS += -I.
CFLAGS += -DARENA_IMPLEMENTATION -DGUI_IMPLEMENTATION -DSIM_IMPLEMENTATION -DDRAW_IMPLEMENTATION -DCAMERA_IMPLEMENTATION -DRAYGUI_IMPLEMENTATION
LDFLAGS = $(shell pkg-config --libs raylib)

all: $(BUILD)/$(TARGET)

$(BUILD)/$(TARGET): FORCE
	mkdir -p $(BUILD)
	$(CC) $(SOURCES) $(CFLAGS) $(LDFLAGS) -o $@

run: all
	./$(BUILD)/$(TARGET)

clean:
	rm -rf $(BUILD)

web:
	# https://anguscheng.com/post/2023-12-12-wasm-game-in-c-raylib/

compile_flags.txt: FORCE
	@echo "Generating compile_flags.txt."
	@echo $(CFLAGS) | tr " " "\n" > $@
FORCE:
