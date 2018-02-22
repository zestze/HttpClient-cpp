TARGET 		= main
CC 		= clang++-5.0
CXX 		= clang++-5.0
INCLUDES 	=
CFLAGS 		= -g -Wall $(INCLUDES)
CXXFLAGS 	= -g -Wall $(INCLUDES) -std=c++14 -pthread
LDFLAGS 	= -g -pthread
LDLIBS 		= -lboost_system

$(TARGET): client.o shared_methods.o

$(TARGET).o: client.h shared_methods.h

client.o: HttpRequest.h shared_methods.h

shared_methods.o: 

.PHONY: clean
clean:
	rm -rf a.out core *.o $(TARGET)
