COMPILER = g++
FLAGS = -std=c++14 -O3 -Wall -Werror #-Wextra

SO_DEPS = $(shell pkg-config --libs --cflags libSimpleAmqpClient msgpack librabbitmq opencv libssl | sed -E 's/rt\\;//g') 
SO_DEPS2 = -lflycapture -I/usr/include/flycapture
A_DEPS = $(shell pkg-config --libs --cflags --static libSimpleAmqpClient msgpack librabbitmq opencv libssl | sed -E 's/rt\\;//g')

SRCS := $(shell find $(SOURCEDIR) -name '*.cpp')
EXECS := $(SRCS:.cpp=)

all: $(EXECS)

clean:
	rm -f $(EXECS)

camera-ptgrey: camera-ptgrey.cpp ../../include/drivers/ptgrey.hpp
	$(COMPILER) $@.cpp -o $@ $(FLAGS) $(SO_DEPS) $(SO_DEPS2) -lboost_program_options -I $(PWD)/../../include -lpthread