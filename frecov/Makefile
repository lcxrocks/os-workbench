NAME := $(shell basename $(PWD))
export MODULE := M5
all: $(NAME)-64 $(NAME)-32
LDFLAGS += -lm

include ../Makefile
.PHONY: run ``
run: all git
	@echo "64-bit:"
	@./$(NAME)-64 ../../M5-frecov.img
	
