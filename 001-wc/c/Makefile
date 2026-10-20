CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror
PYTHON ?= python3
CHALLENGES_DIR ?= ../../../challenges
PROGRAM = build/oxide-wc

.DELETE_ON_ERROR:
.PHONY: all test check clean

all: $(PROGRAM)

$(PROGRAM): main.c
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) "$<" $(LDFLAGS) $(LDLIBS) -o "$@"

test: all
	$(PYTHON) "$(CHALLENGES_DIR)/001-wc/tests/test.py" "$(CURDIR)/$(PROGRAM)"

check:
	$(PYTHON) ../../tools/check_lock.py --checkout "$(CHALLENGES_DIR)"
	$(MAKE) test CHALLENGES_DIR="$(CHALLENGES_DIR)"

clean:
	$(RM) $(PROGRAM)
