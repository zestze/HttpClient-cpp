TARGET 		= main
CC 		= clang++-5.0
CXX 		= clang++-5.0
INCLUDES 	=
CFLAGS 		= -g -Wall $(INCLUDES)
CXXFLAGS 	= -g -Wall $(INCLUDES) -std=c++17 -pthread
LDFLAGS 	= -g -pthread
LDLIBS 		= -lboost_system

$(TARGET): client.o shared.o Conc_Queue.o

$(TARGET).o: client.h shared.h

client.o: HttpRequest.h shared.h Conc_Queue.h

Conc_Queue.o:

shared.o: 

.PHONY: clean
clean:
	rm -rf a.out core *.o $(TARGET)
