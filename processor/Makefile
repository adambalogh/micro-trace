all:
	protoc --proto_path=/home/adam/Developer/tracing/proto --python_out=gen/ /home/adam/Developer/tracing/proto/request_log.proto

run:
	./venv/bin/python processor.py
