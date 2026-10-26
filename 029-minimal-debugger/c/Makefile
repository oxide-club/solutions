CC ?= cc
CFLAGS ?= -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror
PYTHON ?= python3
CHALLENGES_DIR ?= ../../../challenges
CHALLENGE = $(CHALLENGES_DIR)/029-minimal-debugger
PROGRAM = build/oxide-minimal-debugger

.DELETE_ON_ERROR:
.PHONY: all fixtures test check clean

all: $(PROGRAM)

$(PROGRAM): oxdbg.c
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) "$<" $(LDFLAGS) $(LDLIBS) -o "$@"

# As receitas oficiais também compilam os supervisores de teste.
fixtures:
	$(MAKE) -C "$(CHALLENGE)" all

test: all fixtures
	$(PYTHON) "$(CHALLENGE)/tests/check.py" "$(CURDIR)/$(PROGRAM)"

check:
	$(PYTHON) ../../tools/check_lock.py --checkout "$(CHALLENGES_DIR)"
	$(MAKE) test CHALLENGES_DIR="$(CHALLENGES_DIR)"

clean:
	$(RM) $(PROGRAM)
