all:
	thrift --gen js:node log.thrift
	thrift --gen cpp -o ../instrument log.thrift

run:
	npm start
