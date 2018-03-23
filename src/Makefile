TARGET 		= Main
CC 		= clang++-5.0
CXX 		= clang++-5.0
INCLUDES 	=
CFLAGS 		= -g -Wall $(INCLUDES)
#CXXFLAGS 	= -g -Wall $(INCLUDES) -std=c++17 -pthread
CXXFLAGS 	= -g -O2 $(INCLUDES) -std=c++17 -pthread
LDFLAGS 	= -g -pthread
LDLIBS 		= -lcrypto

$(TARGET): Client.o Shared.o

$(TARGET).o: Client.h Shared.h

Client.o: HttpRequest.h Shared.h ByteRange.h

Shared.o: 

.PHONY: clean
clean:
	rm -rf a.out core *.o $(TARGET)
