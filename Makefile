
CC       = gcc
ECHO     = echo "going to compile for target $@"
PROG = decryptor.out
SHARED_FLAGS= -shared -fPIC -pthread

SRCS := $(subst ./,,$(shell find . -maxdepth 1 -name "*.c"))
OBJECTS := $(patsubst %.c, %.out,$(SRCS))

all: $(SRCS) $(OBJECTS)

%.out: %.c
	$(CC) $< -lmta_crypt -lmta_rand -lcrypto -lrt -L`pwd` -o $@

clean:
	rm *.out *.log *.o