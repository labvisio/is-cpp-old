SRCS := $(shell find $(SOURCEDIR) -name '*.cpp')
EXECS := $(SRCS:.cpp=)

SUBDIRS = ptgrey webcam image_viewer

.PHONY: subdirs $(SUBDIRS)

subdirs: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

clean:
	rm -f $(EXECS)