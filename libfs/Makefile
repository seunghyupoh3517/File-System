# Target library
lib 	:= libfs.a
objs 	:= fs.o disk.o

CC 		:= gcc
CFLAGS 	:= -Wall -Wextra -Werror -MMD
CFALGS 	+= g

ifneq ($(V),1)
Q = @
endif

all: $(lib)

deps := $(patsubst %.o,%.d,$(objs))
-include $(deps)

$(lib): $(objs)
	@echo "Create Library"
	$(Q)ar rcs $@ $^

%.o: %.c
	@echo "CC $@"
	$(Q)$(CC) $(CFLAGS) -c -o $@ $<

clean:
	@echo "CLEAN"
	$(Q)rm -f $(lib) $(objs) $(deps)
