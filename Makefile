TARGET 		= main
CC 		= clang++-5.0
CXX 		= clang++-5.0
INCLUDES 	=
CFLAGS 		= -g -Wall $(INCLUDES)
CXXFLAGS 	= -g -Wall $(INCLUDES) -std=c++17 -pthread
#CXXFLAGS 	= -g -O2 $(INCLUDES) -std=c++17 -pthread
LDFLAGS 	= -g -pthread
LDLIBS 		= -lboost_system

$(TARGET): client.o shared.o ConcQueue.o

$(TARGET).o: client.h shared.h

client.o: HttpRequest.h shared.h ConcQueue.h ByteRange.h

ConcQueue.o:

shared.o: 

.PHONY: clean
clean:
	rm -rf a.out core *.o $(TARGET)
