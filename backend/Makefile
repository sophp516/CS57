# Backend: register allocation + assembly generation
# Build: backend_driver (reads .ll, writes .s)

CC = gcc
CXX = g++
LLVM_CONFIG = llvm-config-17

CFLAGS = -Wall -g -I. -I /usr/include/llvm-c-17/
LDFLAGS = $(shell $(LLVM_CONFIG) --ldflags --libs core)

OBJS = register_alloc.o assembly_gen.o backend_driver.o

backend_driver: $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

register_alloc.o: register_alloc.c register_alloc.h
	$(CC) $(CFLAGS) -c register_alloc.c -o register_alloc.o

assembly_gen.o: assembly_gen.c assembly_gen.h register_alloc.h
	$(CC) $(CFLAGS) -c assembly_gen.c -o assembly_gen.o

backend_driver.o: backend_driver.c register_alloc.h assembly_gen.h
	$(CC) $(CFLAGS) -c backend_driver.c -o backend_driver.o

clean:
	rm -f $(OBJS) backend_driver

.PHONY: clean
