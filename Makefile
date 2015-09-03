LIBJIT_PATH = /usr/local/lib/
LIBJIT_INCLUDE_PATH = /usr/local/include/jit/jit
LIBJIT_AR = $(LIBJIT_PATH)/libjit.a

CC = gcc
LD = gcc
CCOPT = -g -O0
CCFLAGS = -c $(CCOPT)
LDFLAGS = -lpthread -lm -ldl

vm: vm.o
	$(LD) $^ $(LIBJIT_AR) $(LDFLAGS) -o $@

vm.o: vm.c
	$(CC) -I$(LIBJIT_INCLUDE_PATH) -I. $(CCFLAGS) $^ -o $@
