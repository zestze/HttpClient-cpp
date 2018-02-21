TARGET 		= client
CC 		= clang++-5.0
CXX 		= clang++-5.0
INCLUDES 	=
CFLAGS 		= -g -Wall $(INCLUDES)
CXXFLAGS 	= -g -Wall $(INCLUDES) -std=c++14 -pthread
LDFLAGS 	= -g -pthread
LDLIBS 		= -lboost_system

$(TARGET):

$(TARGET).o: HttpRequest.h

.PHONY: clean
clean:
	rm -rf a.out core *.o $(TARGET)
