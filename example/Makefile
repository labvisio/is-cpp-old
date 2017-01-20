SRCS := $(shell find $(SOURCEDIR) -name '*.cpp')
EXECS := $(SRCS:.cpp=)

SUBDIRS = control_robot image_viewer odometry_viewer ptgrey sync_request final_position_controller pattern_viewer webcam find_pattern pioneer sync visual_control

.PHONY: subdirs $(SUBDIRS)

subdirs: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

clean:
	rm -f $(EXECS)