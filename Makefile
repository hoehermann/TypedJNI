JDK_HOME?=/usr/lib/jvm/java-11-openjdk-amd64

.phony: run_java run clean

all: typedjnitest TypedJNITest.class

TypedJNITest.class: TypedJNITest.java
	javac TypedJNITest.java

typedjni.o: typedjni.cpp
	g++ -c typedjni.cpp -I$(JDK_HOME)/include/ -I$(JDK_HOME)/include/linux

typedjnitest: test.cpp typedjni.o
	g++ -o typedjnitest -g -ggdb test.cpp typedjni.o -I$(JDK_HOME)/include/ -I$(JDK_HOME)/include/linux -L$(JDK_HOME)/lib/server -ljvm

run_java: TypedJNITest.class
	java -classpath . TypedJNITest

run: typedjnitest TypedJNITest.class
	env LD_LIBRARY_PATH=$(JDK_HOME)/lib/server ./typedjnitest

clean:
	rm -f TypedJNITest.class typedjni.o
