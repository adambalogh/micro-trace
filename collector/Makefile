all:
	thrift --gen js:node log.thrift
	thrift --gen cpp -o ../instrument log.thrift

run:
	PGUSER=dciohjpo \
	PGHOST=horton.elephantsql.com \
    PGPASSWORD=yjfkAOIhJwx1JE-txPs1tFsdQ547RkPo \
	PGDATABASE=dciohjpo \
	PGPORT=5432 \
	npm start
