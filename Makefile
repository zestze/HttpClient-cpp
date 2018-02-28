TARGET 		= Main
CC 		= clang++-5.0
CXX 		= clang++-5.0
INCLUDES 	=
CFLAGS 		= -g -Wall $(INCLUDES)
CXXFLAGS 	= -g -Wall $(INCLUDES) -std=c++17 -pthread
#CXXFLAGS 	= -g -O2 $(INCLUDES) -std=c++17 -pthread
LDFLAGS 	= -g -pthread
LDLIBS 		= -lboost_system

$(TARGET): Client.o shared.o base64.o

$(TARGET).o: Client.h shared.h

Client.o: HttpRequest.h shared.h ConcQueue.h ByteRange.h base64.h

shared.o: 

base64.0:

.PHONY: clean
clean:
	rm -rf a.out core *.o $(TARGET)
