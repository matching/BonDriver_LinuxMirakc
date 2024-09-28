CXX := g++
INCLUDES := -Iinclude
CPPFLAGS := -MMD
CXXFLAGS := -std=c++17 $(INCLUDES) -O2 -Wall -pthread -fPIC
LDFLAGS := -shared -pthread
LDLIBS := -lm -ldl

TARGET := BonDriver_LinuxMirakc.so
OBJS := src/BonDriver_LinuxMirakc.o src/char_code_conv.o src/GrabTsData.o src/config.o src/util.o
DEPS := $(OBJS:.o=.d)

all: $(TARGET)

clean:
	$(RM) -v $(TARGET) $(OBJS) $(DEPS)

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

-include $(DEPS)

.PHONY: all clean
