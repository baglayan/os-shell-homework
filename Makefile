CC     := clang
CFLAGS := -O2 \
		  -Wall -Werror -Wextra -Wpedantic -Wformat=2 -Wformat-overflow=2 -Wformat-truncation=2 -Wformat-security -Wnull-dereference -Wstack-protector -Wtrampolines -Walloca -Wvla -Warray-bounds=2 -Wimplicit-fallthrough=3 -Wtraditional-conversion -Wshift-overflow=2 -Wcast-qual -Wstringop-overflow=4 -Wconversion -Warith-conversion -Wlogical-op -Wduplicated-cond -Wduplicated-branches -Wformat-signedness -Wshadow -Wstrict-overflow=4 -Wundef -Wstrict-prototypes -Wswitch-default -Wswitch-enum -Wstack-usage=1000000 -Wcast-align=strict \
		  -Wno-format-nonliteral -Wno-unknown-warning-option -Wno-unused-parameter -Wno-unused-command-line-argument -Wno-unused-but-set-variable\
		  -D_FORTIFY_SOURCE=3 \
		  -fstack-protector-strong -fstack-clash-protection -fPIE \
		  -fsanitize=bounds -fsanitize-undefined-trap-on-error \
		  -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack -Wl,-z,separate-code \

OS     := $(shell uname -s)

SRCS   := hwsh.c 

OBJS   := ${SRCS:c=o}
PROGS  := ${SRCS:.c=}

.PHONY: all
all: ${PROGS}

${PROGS} : % : %.o Makefile
	${CC} $< -o $@

clean:
	rm -f ${PROGS} ${OBJS}

%.o: %.c Makefile
	${CC} ${CFLAGS} -c $<
