ELF_NAME := mixup

DIR_BIN := bin
DIR_BLD := .build
DIR_SRC := src

DIR_GUARD = mkdir -p $(@D)

CC := clang
LIBS := -lm -lmongoose -luuid
PKG_CONFIG := libpipewire-0.3
CFLAGS := -g -O0 -Wextra -std=gnu23
DEFNS := 

TARGET := $(DIR_BIN)/$(ELF_NAME)
HEADERS := $(wildcard $(DIR_SRC)/*.h)
BINARIES := $(patsubst $(DIR_SRC)/%.c, $(DIR_BLD)/%.o, $(wildcard $(DIR_SRC)/*.c))

$(TARGET): $(BINARIES)
	$(DIR_GUARD)
	$(CC) $^ $(shell pkg-config --libs $(PKG_CONFIG)) $(LIBS) -o $@

$(DIR_BLD)/%.o: $(DIR_SRC)/%.c $(HEADERS)
	$(DIR_GUARD)
	$(CC) $(DEFNS) $(CFLAGS) $(shell pkg-config --cflags $(PKG_CONFIG)) -c $< -o $@ 

.PHONY: run clean

run: $(TARGET)
	$(TARGET)

clean:
	rm -rf $(DIR_BLD)
	rm -rf $(DIR_BIN)
