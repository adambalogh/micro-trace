compile:
	gcc -shared -fPIC -I$(shell pwd)/lib trace.c -o trace.so

run:
	LD_PRELOAD=$(shell pwd)/trace.so python helloworld.py
