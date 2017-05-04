compile:
	gcc -Wall -shared -fPIC -I$(shell pwd)/lib trace.c -o trace.so

cecho:
	gcc -Wall apps/echo.c -levent -levent_core -L /usr/local/lib -o apps/echo

echo: cecho
	./apps/echo

tornado:
	LD_PRELOAD=$(shell pwd)/trace.so python apps/helloworld.py

aio:
	LD_PRELOAD=$(shell pwd)/trace.so python3 apps/async.py
