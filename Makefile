include makefile.globals

PROCESSES_DIR := processes

# It breakes because of fucking spaces anywhere in the file path................................
PROCESS_NAMES := $(foreach proc,$(wildcard $(PROCESSES_DIR)/*/),$(lastword $(subst /, ,$(proc))))

all: $(PROCESS_NAMES)

$(PROCESS_NAMES):
	$(MAKE) -C $(PROCESSES_DIR)/$@ all

clean:
	$(MAKE) -C $(UTILS_BASE_DIR) clean
	@for dir in $(PROCESS_NAMES); do \
		$(MAKE) -C $(PROCESSES_DIR)/$$dir clean; \
	done

.PHONY: all clean
