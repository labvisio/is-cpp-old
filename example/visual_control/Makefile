COMPILER = g++
FLAGS = -std=c++14 -O3 -Wall -Werror -Wextra

SO_DEPS = $(shell pkg-config --libs --cflags libSimpleAmqpClient msgpack librabbitmq opencv)
SO_DEPS += -lboost_program_options -lpthread -larmadillo

all: visual_control

visual_control: visual_control.cpp 
	$(COMPILER) $^ -o $@ $(FLAGS) $(SO_DEPS) 
