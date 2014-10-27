target=client
GCC=g++
CFLAGS =
all:
	$(GCC) $(CFLAGS) test.cc redis_client.cc libhiredis.a -o client 
