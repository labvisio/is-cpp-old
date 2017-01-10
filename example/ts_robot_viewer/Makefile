COMPILER = g++
FLAGS = -std=c++14 -O3 #-Wall -Werror -Wextra

SO_DEPS = $(shell pkg-config --libs --cflags libSimpleAmqpClient msgpack librabbitmq)
SO_DEPS += -lboost_program_options -I $(PWD)/../../include -lpthread

SRCS := $(shell find $(SOURCEDIR) -name '*.cpp')
EXECS := $(SRCS:.cpp=)

all: ts_robot_viewer

clean:
	rm -f $(EXECS)

ts_robot_viewer: ts_robot_viewer.cpp
	$(COMPILER) $^ -o $@ $(FLAGS) $(SO_DEPS)