CC ?= gcc

BUILD := .build
OUT := epi-gimp

SRC := $(shell find src -type f -name "*.c")
OBJS := $(SRC:%.c=$(BUILD)/%.o)

LIBS := gtk+-3.0
$(info $(LIBS))

CFLAGS += $(shell cat warning_flags.txt)
CFLAGS += -O2
CFLAGS += -iquote src
CFLAGS += $(shell pkg-config --cflags $(LIBS))

LDLIBS += -lm $(shell pkg-config --libs $(LIBS))

.PHONY: all
all: $(OUT)

$(BUILD)/%.o: %.c
	mkdir -p $(dir $@)
	$(COMPILE.c) -o $@ $<

$(OUT): $(OBJS)
	$(LINK.c) -o $@ $^ $(LDLIBS)

.PHONY: clean
clean:
	$(RM) $(OBJS)

.PHONY: fclean
fclean: clean
	$(RM) $(OUT)

.PHONY: re
.NOTPARALLEL: re
re: fclean all

PREFIX ?= /usr
BINDIR := $(PREFIX)/bin

.PHONY: install
install:
	mkdir -p $(BINDIR)
	install -Dm577 $(OUT) -t $(BINDIR)
