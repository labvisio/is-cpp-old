COMPILER = g++
FLAGS = -std=c++14 -O3 -Wall -Werror -Wextra

SO_DEPS = $(shell pkg-config --libs --cflags libSimpleAmqpClient msgpack librabbitmq opencv)
SO_DEPS += -lboost_program_options -lpthread 

all: sync

sync: sync.cpp sync.hpp
	$(COMPILER) $^ -o $@ $(FLAGS) $(SO_DEPS) 