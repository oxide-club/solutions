CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror
PYTHON ?= python3
CHALLENGES_DIR ?= ../../../challenges
PROGRAM = build/oxide-image-transform

.DELETE_ON_ERROR:
.PHONY: all test check clean

all: $(PROGRAM)

CPPFLAGS += -I"$(CHALLENGES_DIR)/008-image-transform/materials"

$(PROGRAM): main.c $(CHALLENGES_DIR)/008-image-transform/materials/ppm.h
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) "$<" $(LDFLAGS) $(LDLIBS) -o "$@"

test: all
	$(PYTHON) "$(CHALLENGES_DIR)/008-image-transform/tests/test.py" "$(CURDIR)/$(PROGRAM)"

check:
	$(PYTHON) ../../tools/check_lock.py --checkout "$(CHALLENGES_DIR)"
	$(MAKE) test CHALLENGES_DIR="$(CHALLENGES_DIR)"

clean:
	$(RM) $(PROGRAM)
