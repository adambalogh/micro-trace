compile:
	javac -d classes -cp jetty-all-uber.jar HelloWorld.java

run:
	java -cp classes:jetty-all-uber.jar org.eclipse.jetty.embedded.HelloWorld

tcpkali:
	tcpkali -em "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n" -T 10s --sndbuf 1 localhost:8080/examples/servlets/servlet/HelloWorldExample

instrument:
	LD_PRELOAD=/usr/local/lib/libtrace.so  java -cp classes:jetty-all-uber.jar org.eclipse.jetty.embedded.HelloWorld


