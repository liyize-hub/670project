#!/bin/bash
# define ip and file/webpage
#ip=localhost
ip=

# compile retriever
g++ -o retriever ./Retriever.cpp

# start retriever
# 401 unauthorized
./retriever /Users/kiddli/Downloads/CSS432HW2-main/other/SecretFile.html
# # 200 ok
# ./retriever 64.227.48.38/content.txt
# # 403 forbidden
# ./retriever 64.227.48.38/../file.html
# # 200 ok
# ./retriever 64.227.48.38/content.txt
# # 404 not found
# ./retriever 64.227.48.38/././/hjklh

