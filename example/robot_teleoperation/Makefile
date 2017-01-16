COMPILER = g++
FLAGS = -std=c++14 -O3 #-Wall -Werror -Wextra

SO_DEPS = $(shell pkg-config --libs --cflags libSimpleAmqpClient msgpack librabbitmq)
SO_DEPS += -lboost_program_options -I $(PWD)/../../include -lpthread -lsfml-system -lsfml-window

SRCS := $(shell find $(SOURCEDIR) -name '*.cpp')
EXECS := $(SRCS:.cpp=)

all: robot_teleoperation

clean:
	rm -f $(EXECS)

robot_teleoperation: robot_teleoperation.cpp
	$(COMPILER) $^ /home/felippe/joystick/joystick.cc  -o $@ $(FLAGS) $(SO_DEPS)