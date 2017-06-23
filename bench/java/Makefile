compile:
	javac -d classes -cp jetty-all-uber.jar HelloWorld.java

run:
	java -cp classes:jetty-all-uber.jar org.eclipse.jetty.embedded.HelloWorld

instrument:
	LD_PRELOAD=/usr/local/lib/libtrace.so  java -cp classes:jetty-all-uber.jar org.eclipse.jetty.embedded.HelloWorld

