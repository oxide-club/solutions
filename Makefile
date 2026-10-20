CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror
PYTHON ?= python3
CHALLENGES_DIR ?= ../challenges
PROGRAMS = build/001-wc build/002-ppm

.DELETE_ON_ERROR:
.PHONY: all test check clean

all: $(PROGRAMS)

build/%: %/reference/main.c
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) "$<" $(LDFLAGS) $(LDLIBS) -o "$@"

test: all
	$(PYTHON) "$(CHALLENGES_DIR)/001-wc/tests/test.py" "$(CURDIR)/build/001-wc"
	$(PYTHON) "$(CHALLENGES_DIR)/002-ppm/tests/test.py" "$(CURDIR)/build/002-ppm"

check:
	$(PYTHON) tools/check_lock.py --checkout "$(CHALLENGES_DIR)"
	$(PYTHON) -m unittest discover -s tests -v
	$(MAKE) test CHALLENGES_DIR="$(CHALLENGES_DIR)"

clean:
	$(RM) $(PROGRAMS)
