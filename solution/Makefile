# TARGET_EXEC ?= assemble

BUILD_DIR ?= ./build
SRC_DIRS ?= ./src

TARGET_ASSEMBLE ?= $(BUILD_DIR)/assemble
TARGET_EMULATE ?= $(BUILD_DIR)/emulate

# SRCS := $(shell find $(SRC_DIRS) -name *.c)

ALL_SRCS = $(shell find $(SRC_DIRS) -name *.c)
# Assembler sources
SRCS_A = $(filter $(SRC_DIRS)/assemble.c $(SRC_DIRS)/assembler/%.c, $(ALL_SRCS))
OBJS_A := $(SRCS_A:%=$(BUILD_DIR)/%.o)
DEPS_A := $(OBJS_A:.o=.d)

# Emulator sources
SRCS_E = $(filter $(SRC_DIRS)/emulate.c $(SRC_DIRS)/emulator/%.c, $(ALL_SRCS))
OBJS_E := $(SRCS_E:%=$(BUILD_DIR)/%.o)
DEPS_E := $(OBJS_E:.o=.d)

SRCS_COMMON := $(filter-out $(SRCS_A) $(SRCS_E), $(ALL_SRCS))
OBJS_COMMON := $(SRCS_COMMON:%=$(BUILD_DIR)/%.o)
DEPS_COMMON := $(OBJS_COMMON:.o=.d)

INC_DIRS := $(shell find $(SRC_DIRS) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

CC ?= gcc
CFLAGS ?= -std=c17 -g -D_POSIX_SOURCE -D_DEFAULT_SOURCE\
	-Wimplicit-fallthrough\
	-Wall\
	-Werror \
	-Wno-error=unused-variable \
	-Wno-error=comment \
	-Wno-error=unused-but-set-variable \
	$(INC_FLAGS) -MMD -MP
	# -Werror=return-type\
	# -Werror=implicit-function-declaration\

all: assemble emulate

$(TARGET_ASSEMBLE): $(OBJS_COMMON) $(OBJS_A)
	$(CC) $(OBJS_COMMON) $(OBJS_A) -o $@ $(LDFLAGS)

$(TARGET_EMULATE): $(OBJS_COMMON) $(OBJS_E)
	$(CC) $(OBJS_COMMON) $(OBJS_E) -o $@ $(LDFLAGS)

# c source
$(BUILD_DIR)/%.c.o: %.c
	$(MKDIR_P) $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: all clean assemble emulate cleantest cleanout test test_folder

assemble: $(TARGET_ASSEMBLE)
	chmod +x $(TARGET_ASSEMBLE)

emulate: $(TARGET_EMULATE)
	chmod +x $(TARGET_EMULATE)

clean:
	$(RM) -r $(BUILD_DIR)

cleanout:
	$(RM) -r ./out

cleantest: 
	$(RM) -r $(TEST_OUT)
	$(RM) -rf $(TEST_DIR)/gen


AARCH64 ?= aarch64-none-elf
ASFLAGS ?= 

TEST_DIR ?= ./test
TEST_OUT ?= ./out/test
TESTS ?= $(wildcard $(TEST_DIR)/*.s)
TESTS_A_EXP ?= $(TESTS:$(TEST_DIR)/%.s=$(TEST_OUT)/%_exp.bin)
TESTS_A_ACT ?= $(TESTS:$(TEST_DIR)/%.s=$(TEST_OUT)/%.bin)

# test: test_folder $(TESTS_A_EXP) test_assemble

test: test_folder
	./test/run_tests.py $(TEST_ARGS)

ifeq (test, $(firstword $(MAKECMDGOALS)))
  TEST_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
  $(eval $(TEST_ARGS):;@:) # turn arguments into do-nothing targets
endif

clean_a_act:
	$(RM) -r $(TESTS_A_ACT)

test_folder:
	$(MKDIR_P) $(TEST_OUT)

$(TESTS_A_EXP): $(TESTS)
	$(AARCH64)-as $(ASFLAGS) $< -o $@

-include $(DEPS_A) $(DEPS_E) $(DEPS_COMMON)

clean_unicorn:
	$(RM) -r $(TEST_DIR)/emulator_exp

MKDIR_P ?= mkdir -p
