ifndef GAP_SDK_HOME
  $(error Source sourceme in gap_sdk first)
endif
MODEL_PREFIX = classification


io=uart
PMSIS_OS = freertos

APP_CFLAGS += -DMODEL_QUANTIZED

# load the model pre-quantized by TensorFlow
# if set to false, will quantize using images from the /samples folder
MODEL_PREQUANTIZED = true

ifeq "$(MODEL_PREQUANTIZED)" "true"
  NNTOOL_EXTRA_FLAGS = -q
  NNTOOL_SCRIPT=model/nntool_script
else
  NNTOOL_SCRIPT=model/nntool_script_q
endif

QUANT_BITS=8
BUILD_DIR=BUILD
MODEL_SQ8=1

$(info Building GAP8 mode with $(QUANT_BITS) bit quantization)

MODEL_SUFFIX = _SQ8BIT

include model_decl.mk
TRAINED_MODEL=model/test_model.tflite
CLUSTER_STACK_SIZE?=6096
CLUSTER_SLAVE_STACK_SIZE?=1024
TOTAL_STACK_SIZE=$(shell expr $(CLUSTER_STACK_SIZE) \+ $(CLUSTER_SLAVE_STACK_SIZE) \* 7)
MODEL_L1_MEMORY=$(shell expr 60000 \- $(TOTAL_STACK_SIZE))
MODEL_L2_MEMORY=270000
MODEL_L3_MEMORY=8000000
FREQ_CL=170
FREQ_FC=200

CPX_TXQ_SIZE=20
CPX_RXQ_SIZE=20

MODEL_L3_EXEC=hram
MODEL_L3_CONST=hflash

pulpChip = GAP
PULP_APP = classification
USE_PMSIS_BSP=1

APP = experimentt
APP_SRCS += minimal_network_code.c inc/lib/cpx/src/com.c inc/lib/cpx/src/cpx.c $(MODEL_GEN_C) $(MODEL_COMMON_SRCS) $(CNN_LIB) 
#APP_CFLAGS += -O3 -g
APP_INC=inc/lib/cpx/inc
#APP_CFLAGS += -DconfigUSE_TIMERS=1 -DINCLUDE_xTimerPendFunctionCall=1
APP_CFLAGS += -g -Os -mno-memcpy -fno-tree-loop-distribute-patterns
APP_CFLAGS += -I. -I$(MODEL_COMMON_INC) -I$(TILER_EMU_INC) -I$(TILER_INC) $(CNN_LIB_INCLUDE) -I$(realpath $(MODEL_BUILD))
APP_CFLAGS += -DPERF -DAT_MODEL_PREFIX=$(MODEL_PREFIX) $(MODEL_SIZE_CFLAGS)
APP_CFLAGS += -DSTACK_SIZE=$(CLUSTER_STACK_SIZE) -DSLAVE_STACK_SIZE=$(CLUSTER_SLAVE_STACK_SIZE)
APP_CFLAGS += -DconfigUSE_TIMERS=1 -DINCLUDE_xTimerPendFunctionCall=1 -DFS_PARTITIONTABLE_OFFSET=0x00000
APP_CFLAGS +=  -DFREQ_FC=$(FREQ_FC) -DFREQ_CL=$(FREQ_CL) -DTXQ_SIZE=$(CPX_TXQ_SIZE) -DRXQ_SIZE=$(CPX_RXQ_SIZE) 


READFS_FILES=$(abspath $(MODEL_TENSORS))

all:: model

clean:: clean_model

include model_rules.mk
$(info APP_SRCS... $(APP_SRCS))
$(info APP_CFLAGS... $(APP_CFLAGS))


RUNNER_CONFIG = $(CURDIR)/config.ini


include $(RULES_DIR)/pmsis_rules.mk
