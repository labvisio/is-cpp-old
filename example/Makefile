COMPILER = g++
FLAGS = -std=c++14 -O3 -Wall -Werror -Wextra

SO_DEPS = $(shell pkg-config --libs --cflags libSimpleAmqpClient msgpack librabbitmq opencv libssl | sed -E 's/rt\\;//g') 
A_DEPS = $(shell pkg-config --libs --cflags --static libSimpleAmqpClient msgpack librabbitmq opencv libssl | sed -E 's/rt\\;//g')

SRCS := $(shell find $(SOURCEDIR) -name '*.cpp')
EXECS := $(SRCS:.cpp=)

all: $(EXECS)


clean:
	rm -f $(EXECS)

%: %.cpp
	$(COMPILER) $@.cpp -o $@ $(FLAGS) $(SO_DEPS) -lboost_program_options -I $(PWD)/../include -lpthread