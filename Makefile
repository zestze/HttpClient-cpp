TARGET 		= client
CC 		= g++
CXX 		= g++
INCLUDES 	=
CFLAGS 		= -g -Wall $(INCLUDES)
CXXFLAGS 	= -g -Wall $(INCLUDES) -std=c++14 -pthread
LDFLAGS 	= -g -pthread
LDLIBS 		= -lboost_system

$(TARGET):

.PHONY: clean
clean:
	rm -rf a.out core *.o $(TARGET)
