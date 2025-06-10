#!/bin/bash

g++ -o test1 test1.cc cputrace.cc
g++ test2.cc cputrace.cc -o test2 -lpthread
g++ test3.cc cputrace.cc -o test3 -lpthread
