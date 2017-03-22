COMPILER = g++
FLAGS = -std=c++14 -O3 -Wall -Werror -Wextra

SO_DEPS = $(shell pkg-config --libs --cflags libSimpleAmqpClient msgpack librabbitmq opencv theoradec theoraenc)
SO_DEPS += -lboost_program_options -lpthread 

all: service cam-pub cam-sub 

clean:
	rm service cam-pub cam-sub

service: service.cpp 
	$(COMPILER) $^ -o $@ $(FLAGS) $(SO_DEPS) 

cam-pub: cam-pub.cpp 
	$(COMPILER) $^ -o $@ $(FLAGS) $(SO_DEPS) 

cam-sub: cam-sub.cpp 
	$(COMPILER) $^ -o $@ $(FLAGS) $(SO_DEPS)