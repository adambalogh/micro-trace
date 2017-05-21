all:
	make -C instrument

export FLASK_APP=apps/backend.py

LIB = instrument/build/trace.so

node: 
	LD_PRELOAD=$(LIB) node apps/frontend.js & LD_PRELOAD=$(LIB) node apps/backend.js & LD_PRELOAD=$(LIB) flask run && fg
