compile:
	gcc -shared -fPIC recv.c -o recv.so

run:
	LD_PRELOAD=$(shell pwd)/recv.so python helloworld.py
