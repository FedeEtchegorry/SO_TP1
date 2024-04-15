include makefile.globals

PROCESSES_DIR := processes

# It breakes because of fucking spaces anywhere in the file path................................
PROCESS_NAMES := $(foreach proc,$(wildcard $(PROCESSES_DIR)/*/),$(lastword $(subst /, ,$(proc))))

all: $(BIN_BASE_DIR) $(PROCESS_NAMES)

$(PROCESS_NAMES):
	$(MAKE) -C $(PROCESSES_DIR)/$@ -f $(MAKE_PROC) all

$(BIN_BASE_DIR):
	mkdir $(@)

clean:
	$(MAKE) -C $(UTILS_BASE_DIR) -f $(MAKE_UTILS) clean
	@for dir in $(PROCESS_NAMES); do \
		$(MAKE) -C $(PROCESSES_DIR)/$$dir -f $(MAKE_PROC) clean; \
	done

.PHONY: all clean
