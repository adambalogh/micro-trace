compile:
	gcc -Wall -shared -fPIC -L/usr/local/lib -lhttp_parser -I$(shell pwd)/lib socket.c trace.c -o trace.so -lpthread

cecho:
	gcc -Wall apps/echo.c -levent -levent_core -L /usr/local/lib -o apps/echo

echo: cecho
	./apps/echo

tornado:
	LD_PRELOAD=$(shell pwd)/trace.so python apps/helloworld.py

aio:
	LD_PRELOAD=$(shell pwd)/trace.so python3 apps/async.py

node:
	@LD_PRELOAD=$(shell pwd)/trace.so node apps/frontend.js & \
	LD_PRELOAD=$(shell pwd)/trace.so node apps/backend.js && \
	fg
