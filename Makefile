CC := gcc
CXX := g++
RM := rm

GCCPLUGINS_DIR := $(shell $(CC) -print-file-name=plugin)
PLUGIN_FLAGS := -I$(GCCPLUGINS_DIR)/include -I$(GCCPLUGINS_DIR)/include/c-family #-Wno-unused-parameter -Wno-unused-variable #-fdump-passes
DESTDIR :=
LDFLAGS :=

BIN := mpxk.so

OBJ := mpxk.o mpxk_builtins.o
OBJ += mpxk_pass_wrappers.o
OBJ += mpxk_pass_bnd_store.o
OBJ += mpxk_pass_rm_bndstx.o
OBJ += mpxk_pass_cfun_args.o

SRC := $(OBJ:.o=.c)

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

all: $(BIN)

$(BIN): $(OBJ)
	$(PLUGINCC) $(PLUGIN_FLAGS) -o $@ $^

%.o : %.c
	$(PLUGINCC) $(PLUGIN_FLAGS) -o $@ -c $<

run test: $(BIN)
	$(CC) -fplugin=$(CURDIR)/$(BIN) test.c -o test -O2 -fdump-tree-all -fdump-ipa-all -fno-inline

clean:
	$(RM) -f $(BIN) test test.c.* test.ltrans0.* test.wpa.* test_*.c.* test_* *.o
