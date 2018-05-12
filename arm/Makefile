# output binary
BIN := epilepsia

# source files
SRCS := opcserver.cpp leddriver.cpp main.cpp

# intermediate directory for generated object files
OBJDIR := .o

# intermediate directory for generated dependency files
DEPDIR := .d

# object files, auto generated from source files
OBJS := $(patsubst %,$(OBJDIR)/%.o,$(basename $(SRCS)))

# dependency files, auto generated from source files
DEPS := $(patsubst %,$(DEPDIR)/%.d,$(basename $(SRCS)))

# compilers (at least gcc and clang) don't create the subdirectories automatically
$(shell mkdir -p $(dir $(OBJS)) >/dev/null)
$(shell mkdir -p $(dir $(DEPS)) >/dev/null)

# C++ compiler
CXX := arm-linux-gnueabihf-g++

# C++ flags
CXXFLAGS := -Wall -Wextra -O3 -std=c++14 -pthread
CXXFLAGS += -static-libstdc++ -static-libgcc
CXXFLAGS += -ffast-math -funroll-loops
CXXFLAGS += -Ithird_parties

# Hack for travis
# Travis only runs trusty, and trusty has no arm toolchains that supports C++14
# So to let travis build for x86 we have to remove platform specific flags
ifneq ($(HOST),x86)
	CXXFLAGS += -march=armv7-a -mtune=cortex-a8 # -mfloat-abi=hard -mfpu=neon
endif

# flags required for dependency generation; passed to compilers
DEPFLAGS = -MT $@ -MD -MP -MF $(DEPDIR)/$*.Td

# compile C++ source files
COMPILE.cc = $(CXX) $(DEPFLAGS) $(CXXFLAGS) -c -o $@

# postcompile step
POSTCOMPILE = mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d

all: $(BIN)

debug: CXXFLAGS += -g
debug: $(BIN)

.PHONY: clean
clean:
	$(RM) -r $(OBJDIR) $(DEPDIR)


$(BIN): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJDIR)/%.o: %.cpp
$(OBJDIR)/%.o: %.cpp $(DEPDIR)/%.d
	$(COMPILE.cc) $<
	$(POSTCOMPILE)

.PRECIOUS = $(DEPDIR)/%.d
$(DEPDIR)/%.d: ;

-include $(DEPS)
