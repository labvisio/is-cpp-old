COMPILER = g++
FLAGS = -std=c++14 -O3 -Wall -Werror -Wextra

SO_DEPS = $(shell pkg-config --libs --cflags libSimpleAmqpClient msgpack librabbitmq opencv theoradec theoraenc)
SO_DEPS += -lboost_program_options -lpthread 

all: {{APP_NAME}}

clean:
	rm {{APP_NAME}}

{{APP_NAME}}: {{APP_NAME}}.cpp 
	$(COMPILER) $^ -o $@ $(FLAGS) $(SO_DEPS) 
