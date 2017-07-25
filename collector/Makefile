all:
	thrift --gen py log.thrift
	thrift --gen cpp -o ../instrument log.thrift

run:
	./venv/bin/python main.py
