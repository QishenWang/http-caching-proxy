CCFLAGS=-std=c++11 -pedantic -Wall -Werror -ggdb3 -pthread
PROGS=server
OBJS=$(patsubst %,%.o,$(PROGS))
all: $(PROGS)

server: server.o Proxy.o RequestParser.o ResponseParser.o Helper.o CacheInfo.o Cache.o ThreadObj.o
	g++ $(CCFLAGS) -o $@ $^ 
%.o: %.cpp %.h
	g++ $(CCFLAGS) -c $<

.PHONY:
	clean
clean:
	rm -f *~ $(PROGS) $(OBJS) server.o Proxy.o RequestParser.o ResponseParser.o Helper.o CacheInfo.o Cache.o ThreadObj.o