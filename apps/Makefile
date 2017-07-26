all:
	cp ../instrument/build/libtrace.so first/.
	cp ../instrument/build/libtrace.so second/.
	cp ../instrument/build/libtrace.so third/.
	docker-compose build
	docker build -t kukikukac/first_app first
	docker build -t kukikukac/second_app second
	docker build -t kukikukac/third_app third
	docker push kukikukac/first_app
	docker push kukikukac/second_app
	docker push kukikukac/third_app

run:
	docker-compose up
