#!/bin/bash

g++ -o test1 test1.cc perfmon.cc
g++ test2.cc perfmon.cc -o test2 -lpthread
