#
# Author: Seth Kuipers
# Class: CIS*2750
# Due Date: February 4, 2018
# Project: Assignment One
#

CC = gcc
CFLAGS = -Wall -std=c11 -g

all: list parser singleList clean

#main: bin/libcal.so bin/liblist.so src/main.c
#	$(CC) $(CFLAGS) -Iinclude src/main.c bin/libcal.so bin/liblist.so -o main

parser: src/CalendarParser.c include/CalendarParser.h include/HelperFunctions.h
	$(CC) $(CFLAGS) -Iinclude -fpic -c src/CalendarParser.c -shared -o ../libcal.so

list: src/LinkedListAPI.c include/LinkedListAPI.h
	$(CC) $(CFLAGS) -Iinclude -c -fpic src/LinkedListAPI.c -shared -o ../liblist.so

linked.o: src/LinkedListAPI.c include/LinkedListAPI.h
	$(CC) $(CFLAGS) -Iinclude -c -fpic src/LinkedListAPI.c -shared -o ./linked.o

cal.o: src/CalendarParser.c include/CalendarParser.h include/HelperFunctions.h
	$(CC) $(CFLAGS) -Iinclude -fpic -c src/CalendarParser.c -shared -o ./cal.o

#singleList: src/CalendarParser.c src/LinkedListAPI.c include/CalendarParser.h include/HelperFunctions.h include/LinkedListAPI.h
#	$(CC) $(CFLAGS) -Iinclude -shared -fpic -c src/CalendarParser.c src/LinkedListAPI.c -o ../lib.so

singleList: linked.o cal.o
	$(CC) $(CFLAGS) -Iinclude -fpic -shared -o ../lib.so linked.o cal.o

clean:
	rm -rf ./*.o
