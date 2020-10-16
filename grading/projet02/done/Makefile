## ======================================================================
## partial Makefile provided to students
##

GTK_INCLUDE := `pkg-config --cflags gtk+-3.0`
GTK_LIBS := `pkg-config --libs gtk+-3.0`

.PHONY: clean new style feedback submit1 submit2 submit

CFLAGS += -std=c11 -Wall -Wextra -pedantic -g

# a bit more checks if you'd like to (uncomment
# CFLAGS += -Wextra -Wfloat-equal -Wshadow                         \
# -Wpointer-arith -Wbad-function-cast -Wcast-align -Wwrite-strings \
# -Wconversion -Wunreachable-code

# uncomment if you want to add DEBUG flag
# CPPFLAGS += -DDEBUG

# ----------------------------------------------------------------------
# feel free to update/modifiy this part as you wish

# all those libs are required on Debian, feel free to adapt it to your box
LDLIBS += -lcheck -lm -lrt -pthread

all::

TARGETS := test-cpu-week08 test-gameboy
CHECK_TARGETS := unit-test-bit unit-test-alu unit-test-bus unit-test-memory \
 unit-test-component unit-test-cpu unit-test-cpu-dispatch-week08 \
 unit-test-cpu-dispatch-week09 unit-test-cartridge unit-test-timer
OBJS =
OBJS_NO_STATIC_TESTS =
OBJS_STATIC_TESTS = alu.o bit.o bootrom.o bus.o cartridge.o component.o cpu.o \
 cpu-alu.o cpu-registers.o cpu-storage.o error.o gameboy.o memory.o opcode.o \
 timer.o
OBJS = $(OBJS_STATIC_TESTS) $(OBJS_NO_STATIC_TESTS)

alu.o: alu.c alu.h bit.h error.h
bit.o: bit.c bit.h error.h
bootrom.o: bootrom.c bootrom.h bus.h memory.h component.h gameboy.h cpu.h \
 alu.h bit.h opcode.h timer.h cartridge.h error.h
bus.o: bus.c bus.h memory.h component.h bit.h error.h
cartridge.o: cartridge.c cartridge.h component.h memory.h bus.h error.h
component.o: component.c component.h memory.h error.h
cpu-alu.o: cpu-alu.c error.h bit.h alu.h cpu-alu.h opcode.h cpu.h bus.h \
 memory.h component.h cpu-storage.h cpu-registers.h
cpu.o: cpu.c error.h cpu.h alu.h bit.h bus.h memory.h component.h \
 opcode.h cpu-alu.h cpu-registers.h cpu-storage.h util.h gameboy.h \
 timer.h cartridge.h
cpu-registers.o: cpu-registers.c cpu-registers.h cpu.h alu.h bit.h bus.h \
 memory.h component.h opcode.h error.h
cpu-storage.o: cpu-storage.c error.h cpu-storage.h memory.h opcode.h \
 bit.h cpu.h alu.h bus.h component.h cpu-registers.h gameboy.h timer.h \
 cartridge.h util.h
error.o: error.c
gameboy.o: gameboy.c gameboy.h bus.h memory.h component.h cpu.h alu.h \
 bit.h opcode.h timer.h cartridge.h error.h bootrom.h
libsid_demo.o: libsid_demo.c sidlib.h
memory.o: memory.c memory.h error.h
opcode.o: opcode.c opcode.h bit.h
sidlib.o: sidlib.c sidlib.h
timer.o: timer.c timer.h component.h memory.h bit.h cpu.h alu.h bus.h \
 opcode.h error.h cpu-storage.h
util.o: util.c

$(TARGETS): $(OBJS)

$(filter-out unit-test-cpu-dispatch-week08 unit-test-cpu-dispatch-week09, $(CHECK_TARGETS)): $(OBJS)
unit-test-cpu-dispatch-week08 unit-test-cpu-dispatch-week09: $(filter-out cpu.o, $(OBJS))

# ----------------------------------------------------------------------
# This part is to make your life easier. See handouts how to make use of it.


clean::
	-@/bin/rm -f *.o *~ $(CHECK_TARGETS) $(TARGETS)

new: clean all

static-check:
	scan-build -analyze-headers --status-bugs -maxloop 64 make CC=clang new

style:
	astyle -n -o -A8 -xt0 *.[ch]

check:: $(CHECK_TARGETS)
	$(foreach target,$(CHECK_TARGETS),./$(target) &&) true

# target to run tests
check:: all
	@if ls tests/*.*.sh 1> /dev/null 2>&1; then \
	  for file in tests/*.*.sh; do [ -x $$file ] || echo "Launching $$file"; ./$$file || exit 1; done; \
	fi

IMAGE=chappeli/feedback:latest
feedback:
	@docker pull $(IMAGE)
	@docker run -it --rm -v ${PWD}:/home/tester/done $(IMAGE)

SUBMIT_SCRIPT=../provided/submit.sh
submit1: $(SUBMIT_SCRIPT)
	@$(SUBMIT_SCRIPT) 1

submit2: $(SUBMIT_SCRIPT)
	@$(SUBMIT_SCRIPT) 2

submit:
	@printf 'what "make submit"??\nIt'\''s either "make submit1" or "make submit2"...\n'

libsid.so: sidlib.c
	$(CC) -fPIC -shared $(CPPFLAGS) $(CFLAGS) $(GTK_INCLUDE) $^ -o $@

libsid_demo.o: CFLAGS += $(GTK_INCLUDE)

libsid_demo: LDFLAGS += -L.
libsid_demo: LDLIBS += $(GTK_LIBS) -lsid
libsid_demo: libsid_demo.o libsid.so
