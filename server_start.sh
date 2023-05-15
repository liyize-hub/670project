#!/bin/bash

# compile server
g++ -o server -std=c++11 ./Server.cpp

# start server
./server 
