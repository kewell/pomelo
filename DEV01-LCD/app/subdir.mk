#config.mk

OUTPUT := built-in.o

ifdef SUBDIR
SUBOBJS := $(addsuffix /$(OUTPUT), $(SUBDIR))
SUBDIR_CLEAN := $(SUBDIR)
SUBMAKE := $(addsuffix /Makefile, $(SUBDIR))
else
SUBOBJS :=
SUBDIR_CLEAN :=
SUBMAKE :=
endif

all: $(SUBMAKE) $(OUTPUT)

-include $(WORK_DIR)/rules.mk

$(OUTPUT): $(OBJS) $(SUBOBJS)
	@echo "  LD  " $(OBJS) $(SUBOBJS)
	@$(LD) -r $^ -o $@
#	$(LD) -r $^ --version-script $(SYMBOLOUT) -o $@
#	$(LD) -r --retain-symbols-file $(SYMBOLOUT) $(OBJS) $(SUBOBJS) -o $@

ifdef SUBDIR
.PHONY: $(SUBMAKE)
$(SUBMAKE):
#	@echo "  "$(MAKE) "-C" $(dir $@)
	$(MAKE) -C $(dir $@)
endif

#ifdef SUBOBJS
#.PHONY: $(SUBOBJS)
#$(SUBOBJS):
#	$(MAKE) -C $(dir $@)
#endif

ifdef SUBDIR
.PHONY: $(SUBDIR_CLEAN)
$(SUBDIR_CLEAN):
	@echo "  "$(MAKE) "-C" $@ "clean"
	$(MAKE) -C $@ clean
endif

.PHONY: clean
clean: $(SUBDIR_CLEAN)
	@rm -f $(OUTPUT) $(OBJS) $(DEPS) *.o .*.c.dep
#	@echo remove objs ok

