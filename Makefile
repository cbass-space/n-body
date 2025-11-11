CC      = clang
TARGET  = main
SOURCES = main.c
BUILD   = build

CFLAGS  = -std=c99 -Wall -g -fcolor-diagnostics -fansi-escape-codes -fsanitize=address
LDFLAGS = $(shell pkg-config --libs raylib)

all: $(BUILD)/$(TARGET)

$(BUILD)/$(TARGET): FORCE
	mkdir -p $(BUILD)
	$(CC) $(SOURCES) $(CFLAGS) $(LDFLAGS) -o $@

run: all
	./$(BUILD)/$(TARGET)

clean:
	rm -rf $(BUILD)

compile_flags.txt: FORCE
	@echo "Generating compile_flags.txt."
	@echo $(CFLAGS) | tr " " "\n" > $@
FORCE:
