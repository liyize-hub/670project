#!/bin/bash
# define ip and file/webpage
#ip=localhost
ip=

# compile retriever
g++ -o retriever ./Retriever.cpp

# start retriever
# 401 unauthorized
./retriever 127.0.0.1/content.txt
# # 200 ok
# ./retriever 127.0.0.1/content.txt
# # 403 forbidden
# ./retriever 127.0.0.1/../file.html
# # 200 ok
# ./retriever 127.0.0.1/content.txt
# # 404 not found
# ./retriever 127.0.0.1/././/hjklh

