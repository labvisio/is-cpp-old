COMPILER = g++
FLAGS = -std=c++14 -O3 #-Wall -Werror -Wextra

SO_DEPS = $(shell pkg-config --libs --cflags libSimpleAmqpClient msgpack librabbitmq)
SO_DEPS += -lboost_program_options -I $(PWD)/../../include -L/usr/local/Aria/lib/ -lAria -ldl -lm -lpthread -lrt

SRCS := $(shell find $(SOURCEDIR) -name '*.cpp')
EXECS := $(SRCS:.cpp=)

all: pioneer

clean:
	rm -f $(EXECS)

pioneer: pioneer.cpp
	$(COMPILER) $^ -o $@ $(FLAGS) $(SO_DEPS)