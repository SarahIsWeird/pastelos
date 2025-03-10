PROGRAMS := $(wildcard */.)
CLEAN_PROGRAMS := $(addprefix clean_,$(PROGRAMS))

.PHONY: all $(PROGRAMS) clean $(CLEAN_PROGRAMS)

$(PROGRAMS):
	@$(MAKE) -C $@

$(CLEAN_PROGRAMS): clean_%: %
	@$(MAKE) -C $< clean

all: $(PROGRAMS)
clean: $(CLEAN_PROGRAMS)
