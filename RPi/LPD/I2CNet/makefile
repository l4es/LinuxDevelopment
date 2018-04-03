SOURCES := $(wildcard *.cpp)
OBJS := $(SOURCES:.cpp=.o)
INCLUDES := -Iinclude
CFLAGS := -std=c++14
LIBS := -lstdc++ -lpthread

all: i2cnet

i2cnet: $(OBJS)
	$(CC) $(CFLAGS) -o i2cnet $(OBJS) $(LFLAGS) $(LIBS)

.cpp.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<
