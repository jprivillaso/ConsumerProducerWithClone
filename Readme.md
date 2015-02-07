# DaFuq Coders #

Parsim - Parallel consumer producer in C++

### What is this repository for? ###

* This project is made for academic purposes. The subject we are developing  
  this project for is called Operating Systems.
* Version 0.0.1

### Dafuq Coders

* Juan Pablo Rivillas
* Camilo Velasquez

### Requirements ###

* GNU make
* GNU GCC 4.8
* Compilation with c++11 and -lpthread

The makefile automatically will compile the source and will put the .o file  
in the /bin folder

### Services start messages ###

* The name of the program that your are going to execute is parsim
* The line of commands is as follows

    ./parsim [-s number [size]] ... [-c defaultSize] [-b backendSize]

### Normal messages

Normal messages are the messages that will determine wich service are  
you going to consume. A message is composed as follows


* message := sequence ':' services ':' number ':' number ':' delay
* sequence := posititeInteger
* services := service | service ',' services
* service := '0' | '1' | '2' | '3' | '4' |'5' |'6' |'7' |'8' |'9'
* number:= integer
* delay := positiveInteger | positiveInteger ',' delay

### Termination code ###

* 0 -> Type 0 when you are testing parsim manually and you want to stop  
  typing messages to test the services
* EOF -> The input file will contain messages typed each message per  
  line as follows

        0:0:1:1:1000
        0:0,1,2:1:19:1000,2000,3000

and so on.

### Input and Output files ###

It is mandatory to have a file called inputs.in. It will automatically create  
a file called *Answers.out* in /src folder if you want, depending on the  
execution typed, described at install.txt

### Headers and Libraries Used ###

* iostream
* algorithm
* sstream -> parsing chains and make validations
* string -> atoi and this methods for using strings
* vector -> for using dynamic vectors
* sched -> for using clone2
* semaphore -> for using semaphores and mutex
* sys/wait -> to use waitPid
* stdio -> to use some special signals
* sys/types
* signal
* cstring