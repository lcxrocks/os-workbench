.PHONY: test libkvdb

all: libkvdb-test-64 libkvdb-test-32

test: libkvdb all
	@echo "==== TEST 64 bit ===="
	@LD_LIBRARY_PATH=.. ./libkvdb-test-64
	
libkvdb-test-64: main.c

libkvdb:
	@cd .. && make -s

libkvdb-test-64: main.c
	gcc -I.. -L.. -m64 main.c -o libkvdb-test-64 -lkvdb-64

libkvdb-test-32: main.c
	gcc -I.. -L.. -m32 main.c -o libkvdb-test-32 -lkvdb-32

clean:
	rm -f libkvdb-test-*
