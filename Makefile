DDK_HOME := ../../che/ddk/ddk

#host or device
CC_SIDE := device

#ASIC or Atlas
CC_PATTERN := Atlas

LOCAL_DIR := ./
INCLUDE_DIR := $(DDK_HOME)/include

LOCAL_MODULE_NAME := sample_dvpp_hlt_ddk

CC_FLAGS := -std=c++11

local_src_files := \
    $(LOCAL_DIR)/sample_dvpp.cpp \

local_inc_dirs := \
	$(INCLUDE_DIR)/inc \
	$(INCLUDE_DIR)/inc/dvpp \
	$(INCLUDE_DIR)/libc_sec/include \
	$(INCLUDE_DIR)/third_party/protobuf/include \


local_shared_libs_dirs := \
	$(DDK_HOME)/$(CC_SIDE)/lib

local_shared_libs := \
	matrixdaemon \
	Dvpp_api \
	Dvpp_vpc \
	OMX_hisi_video_decoder 	\
	OMX_hisi_video_encoder \
	Dvpp_png_decoder \
	Dvpp_jpeg_decoder \
	Dvpp_jpeg_encoder \
	slog \
	c_sec \
        pthread \

include ./cc_rule.mk
