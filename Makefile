target=client
GCC=g++
CFLAGS = -g
all:
	$(GCC) $(CFLAGS) redis_client.cc libhiredis.a -o client 
