compile:
	gcc -shared -fPIC -I$(shell pwd)/lib recv.c -o recv.so

run:
	LD_PRELOAD=$(shell pwd)/recv.so python helloworld.py
