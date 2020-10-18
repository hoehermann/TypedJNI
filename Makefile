JDK_HOME?=/usr/lib/jvm/java-11-openjdk-amd64

.phony: run_java run_cppjava

all: cppjava Java.class

Java.class: Java.java
	javac Java.java

typedjni.o: typedjni.cpp
	g++ -c typedjni.cpp -I$(JDK_HOME)/include/ -I$(JDK_HOME)/include/linux

cppjava: main.cpp typedjni.o
	g++ -o cppjava main.cpp typedjni.o -I$(JDK_HOME)/include/ -I$(JDK_HOME)/include/linux -L$(JDK_HOME)/lib/server -ljvm

run_java: Java.class
	java -classpath . Java

run: cppjava Java.class
	env LD_LIBRARY_PATH=$(JDK_HOME)/lib/server ./cppjava
