#!/bin/bash

# compile server
g++ -o server ./Server.cpp

# start server
./server
echo "Server running"
