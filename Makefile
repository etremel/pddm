CXX = g++
RM = rm -f
MKDIR_P ?= mkdir -p

SRC_DIR := src
BUILD_DIR := build
LIB_DIR := libraries

CPPFLAGS := -std=c++17 -g3 -O0 -Wall
#PROD_FLAGS := -std=c++17 -DNDEBUG -O3

LFLAGS := -Wl,-rpath='$$ORIGIN/../$(LIB_DIR)/mutils',-rpath='$$ORIGIN/../$(LIB_DIR)/mutils-serialization' -L"$(LIB_DIR)/mutils/" -L"$(LIB_DIR)/mutils-serialization/" 
INCLUDES := -I"$(SRC_DIR)/" -I"$(LIB_DIR)/" -I"$(LIB_DIR)/mutils/" -I"$(LIB_DIR)/mutils-serialization/"
LIBS := -lmutils -lmutils-serialization -lpthread -lrt -lcryptopp

#Lazily evaluated: SRCS won't be defined until a target is chosen
OBJS = $(SRCS:$(SRC_DIR)/%=$(BUILD_DIR)/%.o)
DEPS = $(OBJS:.o=.d)


COMMON_SRCS := BftProtocolState.cpp CrusaderAgreementState.cpp CtProtocolState.cpp HftProtocolState.cpp MeterClient.cpp TreeAggregationState.cpp UtilityClient.cpp
COMMON_SRCS := $(addprefix $(SRC_DIR)/,$(COMMON_SRCS))
COMMON_SRCS += $(shell find $(SRC_DIR)/messaging -name *.cpp)
COMMON_SRCS += $(shell find $(SRC_DIR)/util -name *.cpp)

#Target-specific source lists; these will be appended to COMMON_SRCS
ALL_SIM_SRCS := $(shell find $(SRC_DIR)/simulation -name *.cpp)
ALL_SIM_SRCS += $(SRC_DIR)/SimulationMain.cpp

EMULATED_NETWORK_SRCS := EmulatedTestMain.cpp simulation/Meter.cpp simulation/SimParameters.cpp
EMULATED_NETWORK_SRCS := $(addprefix $(SRC_DIR)/,$(EMULATED_NETWORK_SRCS))
EMULATED_NETWORK_SRCS += $(shell find $(SRC_DIR)/networking -name *.cpp)

EMULATED_CRYPTO_SRCS := EmulatedTestWithCrypto.cpp simulation/Meter.cpp simulation/Simparameters.cpp
EMULATED_CRYPTO_SRCS := $(addprefix $(SRC_DIR)/,$(EMULATED_CRYPTO_SRCS))
EMULATED_CRYPTO_SRCS += $(shell find $(SRC_DIR)/networking -name *.cpp)

SIMPLE_MESSAGING_TEST_SRCS := SimpleMessagingTest.cpp 
SIMPLE_MESSAGING_TEST_SRCS := $(addprefix $(SRC_DIR)/,$(SIMPLE_MESSAGING_TEST_SRCS))
SIMPLE_MESSAGING_TEST_SRCS += $(shell find $(SRC_DIR)/networking -name *.cpp)

-include $(DEPS)

#Generic object-from-cpp rule
$(BUILD_DIR)/%.cpp.o: $(SRC_DIR)/%.cpp
	$(MKDIR_P) $(dir $@)
	$(CXX) $(CPPFLAGS) $(INCLUDES) -MMD -c $< -o $@


AllSimulated: SRCS = $(COMMON_SRCS) $(ALL_SIM_SRCS)

.SECONDEXPANSION:
AllSimulated: $$(OBJS)
	$(CXX) $(OBJS) $(LFLAGS) -o $(BUILD_DIR)/$@ $(LIBS)

emulated_network_test: SRCS = $(COMMON_SRCS) $(EMULATED_NETWORK_SRCS)

.SECONDEXPANSION:
emulated_network_test: $$(OBJS)
	$(CXX) $(OBJS) $(LFLAGS) -o $(BUILD_DIR)/$@ $(LIBS)

emulated_with_crypto: SRCS = $(COMMON_SRCS) $(EMULATED_CRYPTO_SRCS)

.SECONDEXPANSION:
emulated_with_crypto: $$(OBJS)
	$(CXX) $(OBJS) $(LFLAGS) -o $(BUILD_DIR)/$@ $(LIBS)

simple_messaging_test: SRCS = $(COMMON_SRCS) $(SIMPLE_MESSAGING_TEST_SRCS)

.SECONDEXPANSION:
simple_messaging_test: $$(OBJS)
	$(CXX) $(OBJS) $(LFLAGS) -o $(BUILD_DIR)/$@ $(LIBS)


.PHONY: clean

clean:
	$(RM) -r $(BUILD_DIR)


