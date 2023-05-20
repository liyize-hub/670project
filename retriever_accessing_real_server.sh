
ip=www.umass.edu/index.html

# compile retriever
g++ -o retriever ./Retriever.cpp

# start retriever
# 200 ok
./retriever $ip
