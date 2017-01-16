COMPILER = g++
FLAGS = -std=c++14 -O3 -Wall -Werror -Wextra

SO_DEPS = $(shell pkg-config --libs --cflags libSimpleAmqpClient msgpack librabbitmq opencv)
SO_DEPS += -lboost_program_options -I $(PWD)/../include -lpthread -lflycapture

SRCS := $(shell find $(SOURCEDIR) -name '*.cpp')
EXECS := $(SRCS:.cpp=)

all: ptgrey/ptgrey webcam/webcam image_viewer/image_viewer ts_viewer/ts_viewer find_pattern/find_pattern

clean:
	rm -f $(EXECS)

ptgrey/ptgrey: ptgrey/ptgrey.cpp ../include/drivers/ptgrey.hpp
	$(COMPILER) $^ -o $@ $(FLAGS) $(SO_DEPS) 

webcam/webcam: webcam/webcam.cpp ../include/drivers/webcam.hpp
	$(COMPILER) $^ -o $@ $(FLAGS) $(SO_DEPS) 

image_viewer/image_viewer: image_viewer/image_viewer.cpp
	$(COMPILER) $^ -o $@ $(FLAGS) $(SO_DEPS)

ts_viewer/ts_viewer: ts_viewer/ts_viewer.cpp
	$(COMPILER) $^ -o $@ $(FLAGS) $(SO_DEPS)
	
find_pattern/find_pattern: find_pattern/find_pattern.cpp find_pattern/find_pattern.hpp
	$(COMPILER) $^ -o $@ $(FLAGS) $(SO_DEPS)