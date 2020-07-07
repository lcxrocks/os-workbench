# CFLAGS = -Wall
# LIBS = -lpthread 

# SRC=$(wildcard *.c)
# DEP=$(wildcard *.h)

# .PHONY: source 
# source: hello stack atexit thread-os-map

# thread-os-map:thread-os-map.c
# 	gcc -o $@ $^ $(CFLAGS) -I. $(AM_HOME) $(LIBS)

# hello: hello-mt.c
# 	gcc -o $@ $^ $(CFLAGS) -I. $(LIBS)

# stack: stack_probe.c
# 	gcc -o $@ $^ $(CFLAGS) -I. $(LIBS)
	
# atexit: atexit_example.c
# 	gcc -o $@ $^ $(CFLAGS) -I. $(LIBS)

# echo:
# 	@echo "src:"
# 	@echo "\t$(SRC)"
# 	@echo "headers:"
# 	@echo "\t$(DEP)"
NAME := thread-os-map
SRCS := thread-os-map.c#$(shell find -L ./src/ -name "thread-os-map.c")
export AM_HOME := $(PWD)/../abstract-machine
ifeq ($(ARCH),)
export ARCH := x86_64-qemu
endif

include $(AM_HOME)/Makefile.app
