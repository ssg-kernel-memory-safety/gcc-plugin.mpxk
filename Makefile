CC := gcc
CXX := g++
RM := rm

GCCPLUGINS_DIR := $(shell $(CC) -print-file-name=plugin)
PLUGIN_FLAGS := -I$(GCCPLUGINS_DIR)/include -I$(GCCPLUGINS_DIR)/include/c-family
DESTDIR :=
LDFLAGS :=

PLUGIN := mpxk.so
OBJ := mpxk.o mpxk_builtins.o
OBJ += mpxk_pass_wrappers.o
OBJ += mpxk_pass_bnd_store.o
OBJ += mpxk_pass_rm_bndstx.o
OBJ += mpxk_pass_cfun_args.o
OBJ += mpxk_pass_sweeper.o
SRC := $(OBJ:.o=.c)

TEST_BIN := test/test_mpxk
MPX_TEST_BIN := test/test_mpx
TEST_DIR := test

TEST_SRC := $(wildcard $(TEST_DIR)/test*.c)
TEST_OBJ := $(TEST_SRC:.c=.mpxk.o) test/mpxk_functions.o test/mock_kernel.o
MPX_TEST_OBJ := $(TEST_SRC:.c=.mpx.o) test/mpx_mock_kernel.o

TEST_DUMPS := $(TEST_OBJ:.mpxk.o=.c.*);

CONFIG_SHELL := $(shell if [ -x "$$BASH" ]; then echo $$BASH; \
		else if [ -x /bin/bash ]; then echo /bin/bash; \
			else echo sh; fi ; fi)

PLUGINCC := $(shell $(CONFIG_SHELL) gcc-plugin.sh "$(CC)" "$(CXX)" "$(CC)")

ifeq ($(PLUGINCC),$(CC))
	PLUGIN_FLAGS += -std=gnu99 -O0
else
	PLUGIN_FLAGS += -std=gnu++98 -fno-rtti -Wno-narrowing -Og
endif

PLUGIN_FLAGS += -fPIC -shared -ggdb -Wall -W -fvisibility=hidden


# Basic MPX configuration flags
MPX_CFLAGS := -mmpx -fcheck-pointer-bounds -fno-chkp-narrow-bounds -fno-chkp-check-read

# Flags using the MPXK plugin
MPXK_CFLAGS := -fplugin=./$(PLUGIN) $(MPX_CFLAGS) -fno-chkp-store-bounds -fno-chkp-use-wrappers

# Flags used to compile support functions for mpxk (e.g. wrappers and load functions).
MPXK_LIB_CFLAGS := $(MPXK_CFLAGS) -fno-chkp-check-write

# Flags needed to "simulate" the Linux kernel KBuild setup (assumed by the mpxk plugin)
KERNEL_FLAGS := -O2 -std=gnu89

# Flags for producing dumps (used to debug tests)
DUMP_FLAGS := -fdump-rtl-all -fdump-tree-all -fdump-ipa-all

all: $(PLUGIN)

$(PLUGIN): $(OBJ)
	$(PLUGINCC) $(PLUGIN_FLAGS) -o $@ $^

%.o: %.c mpxk.h
	$(PLUGINCC) $(PLUGIN_FLAGS) -o $@ -c $<

test: test_mpx test_mpxk
	echo "if objdump -d $(TEST_BIN) | grep -E 'bndstx|bndldx'; then false; else echo 'all OK!'; fi" | bash

test_mpxk: $(TEST_BIN)
	./$(TEST_BIN)

test_mpx: $(MPX_TEST_BIN)
	./$(MPX_TEST_BIN)

$(TEST_BIN): $(PLUGIN) $(TEST_OBJ)
	$(CC) -mmpx -fcheck-pointer-bounds -o $(TEST_BIN) $(TEST_OBJ)

$(MPX_TEST_BIN): $(MPX_TEST_OBJ)
	$(CC) -mmpx -fcheck-pointer-bounds -o $(MPX_TEST_BIN) $(MPX_TEST_OBJ)

# Need to compile MPX / MPX objects separately...
test/%.mpxk.o: $(PLUGIN) test/%.c
	$(CC) $(KERNEL_FLAGS) $(MPXK_CFLAGS) $(DUMP_FLAGS) -o $@ -c $(@:.mpxk.o=.c)

test/%.mpx.o: $(PLUGIN) test/%.c
	$(CC) $(KERNEL_FLAGS) $(MPX_CFLAGS) -o $@ -c $(@:.mpx.o=.c)

# Only MPXK uses the mpxk_functions!
test/mpxk_functions.o: $(PLUGIN) test/mpxk_functions.c
	$(CC) $(KERNEL_FLAGS) $(MPXK_LIB_CFLAGS) -o $@ -c $(@:.o=.c)

# Kernel mockup for mpxk tests compiled without MPX/MPXK
test/mock_kernel.o: $(PLUGIN) test/mock_kernel.c
	$(CC) $(KERNEL_FLAGS) -o $@ -c $(@:.o=.c)

# Need to compile mockup kernel for mpx with mpx to wrap the mock impl.
test/mpx_mock_kernel.o: $(PLUGIN) test/mock_kernel.c
	$(CC) $(KERNEL_FLAGS) $(MPX_CFLAGS) -o $@ -c test/mock_kernel.c

clean:
	$(RM) -f $(PLUGIN) $(TEST_BIN) $(PLUGIN) $(OBJ) $(TEST_OBJ) $(MPX_TEST_OBJ) $(MPX_TEST_BIN) $(TEST_DUMPS)

print-%: ; @echo $* = $($*)
