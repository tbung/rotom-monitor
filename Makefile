DEBUG ?= 0
BUILD_DIR := build

SRC := $(wildcard *.c)
HDR := $(wildcard *.h)
BIN := rotom-monitor

CFLAGS := -Wall -Wextra -std=c99 -lmosquitto -lcjson

ifeq ($(DEBUG), 1)
	CFLAGS += -g -O0 -DDEBUG
	OUT_DIR := build/debug
else
	CFLAGS += -O3
	OUT_DIR := build/release
endif

.PHONY: all
all: $(BIN)

.PHONY: run
run: $(BIN)
	./$(OUT_DIR)/$<

.PHONY: $(BIN)
$(BIN) : % : $(OUT_DIR)/%

$(OUT_DIR)/%: %.c $(HDR) $(OUT_DIR)
	gcc $(CFLAGS) -o $@ $<

$(OUT_DIR):
	mkdir -p $@

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
