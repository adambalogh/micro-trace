all:
	thrift --gen cpp add.thrift
	g++ -std=c++17 -L /usr/local/lib -I gen-cpp gen-cpp/Calculator.cpp client.cc -o client -lthrift
	g++ -std=c++17 -L /usr/local/lib -I gen-cpp gen-cpp/Calculator.cpp first_server.cc -o first_server -lthrift
	g++ -std=c++17 -L /usr/local/lib -I gen-cpp gen-cpp/Calculator.cpp second_server.cc -o second_server -lthrift
