COMPILER = g++
FLAGS = -std=c++14 -O3 -Wall -Werror -Wextra

SO_DEPS = $(shell pkg-config --libs --cflags libSimpleAmqpClient msgpack librabbitmq opencv)
SO_DEPS += -lboost_program_options -lpthread -L/usr/local/include/Aria/lib/ -lAria -ldl -lm -lrt

all: pioneer

pioneer: pioneer.cpp 
	$(COMPILER) $^ -o $@ $(FLAGS) $(SO_DEPS)