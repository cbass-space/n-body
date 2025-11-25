CC      = clang
TARGET  = main
SOURCES = src/main.c
BUILD   = build

CFLAGS  = -std=c99 -Wall -Wextra
CFLAGS += -I.
CFLAGS += -DSIM_IMPLEMENTATION -DPREDICTOR_IMPLEMENTATION -DDRAW_IMPLEMENTATION -DCAMERA_IMPLEMENTATION -DGUI_IMPLEMENTATION -DAPP_IMPLEMENTATION
LDFLAGS = $(shell pkg-config --libs raylib)

DEBUG = -g -fcolor-diagnostics -fansi-escape-codes -fsanitize=address
RELEASE = -O3 -march=native

all: $(BUILD)/$(TARGET)

$(BUILD)/$(TARGET): FORCE
	mkdir -p $(BUILD)
	$(CC) $(SOURCES) $(CFLAGS) $(LDFLAGS) $(DEBUG) -o $@

release: FORCE
	mkdir -p $(BUILD)
	$(CC) $(SOURCES) $(CFLAGS) $(LDFLAGS) $(RELEASE) -o $(BUILD)/release

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
