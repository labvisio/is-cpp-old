COMPILER = g++
FLAGS = -std=c++14 -O3 -Wall -Werror -Wextra

SO_DEPS = $(shell pkg-config --libs --cflags libSimpleAmqpClient msgpack librabbitmq)
SO_DEPS += -lboost_program_options -lpthread

all: odometry_viewer

odometry_viewer: odometry_viewer.cpp 
	$(COMPILER) $^ -o $@ $(FLAGS) $(SO_DEPS)