normal:
	g++ -L /usr/local/lib PracticalSocket.cc Server.cc -o Normal -lpthread

instrumented:
	g++ -L /usr/local/lib PracticalSocket.cc Server.cc -o Instrumented -ltrace -lpthread

tcpkali:
	tcpkali -em "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n" -T 120s --sndbuf 1 localhost:8080
