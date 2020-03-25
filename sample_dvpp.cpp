/**
* @file sample_dvpp.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2012-2018. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "dvpp_config.h"
#include "idvppapi.h"
#include "Venc.h"
#include "Vpc.h"
#include "hiaiengine/c_graph.h"
#include "hiaiengine/ai_memory.h"
#include "hiaiengine/log.h"
#include "hiaiengine/status.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <time.h>
#include <unistd.h>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <deque>
#include <thread>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>

#define MAX_FRAME_NUM_VENC 16

// define the error code
#define USE_DVPP_ERROR 0x6011
enum {
    HIAI_CREATE_DVPP_ERROR_CODE = 0,
    HIAI_JPEGD_CTL_ERROR_CODE,
    HIAI_PNGD_CTL_ERROR_CODE,
    HIAI_JPEGE_CTL_ERROR_CODE,
    HIAI_VDEC_CTL_ERROR_CODE,
    HIAI_VENC_CTL_ERROR_CODE,
    HIAI_VPC_CTL_ERROR_CODE,
    HIAI_OPEN_FILE_ERROR_CODE
};

HIAI_DEF_ERROR_CODE(USE_DVPP_ERROR, HIAI_ERROR, HIAI_CREATE_DVPP_ERROR, \
    "create dvpp error");
HIAI_DEF_ERROR_CODE(USE_DVPP_ERROR, HIAI_ERROR, HIAI_JPEGD_CTL_ERROR, \
    "jpegd ctl error");
HIAI_DEF_ERROR_CODE(USE_DVPP_ERROR, HIAI_ERROR, HIAI_PNGD_CTL_ERROR, \
    "pngd ctl error");
HIAI_DEF_ERROR_CODE(USE_DVPP_ERROR, HIAI_ERROR, HIAI_JPEGE_CTL_ERROR, \
    "jpege ctl error");
HIAI_DEF_ERROR_CODE(USE_DVPP_ERROR, HIAI_ERROR, HIAI_VDEC_CTL_ERROR, \
    "vdec ctl error");
HIAI_DEF_ERROR_CODE(USE_DVPP_ERROR, HIAI_ERROR, HIAI_VENC_CTL_ERROR, \
    "venc ctl error");
HIAI_DEF_ERROR_CODE(USE_DVPP_ERROR, HIAI_ERROR, HIAI_VPC_CTL_ERROR, \
    "vpc ctl error");
HIAI_DEF_ERROR_CODE(USE_DVPP_ERROR, HIAI_ERROR, HIAI_OPEN_FILE_ERROR, \
    "open file error");

using namespace std;
typedef void (*TEST)();
vector<TEST> test_func;

typedef void (*TEST_V)(int);
vector<TEST_V> test_v_func;
//
char in_file_name[500] = "infile";
char out_file_name[500] = "outfile";

//vpc Global variable
int g_width = 0;
int g_high = 0;
int g_hMax = 4095;
int g_vMax = 0;
int g_hMin = 4095;
int g_vMin = 0;
int g_format = 0;
int g_outFormat = 0;
int g_rank = 0;
int g_yuvStoreType = 0;
int g_outWidth = 0;
int g_outHigh = 0;
int g_testType = 1;
int g_loop = 1; // for loop test use.
int g_transform = 0;
int g_threadNum = 1;
uint32_t g_inWidthStride = 0;
uint32_t g_inHighStride = 0;
uint32_t g_outWidthStride = 0;
uint32_t g_outHighStride = 0;
uint32_t g_outLeftOffset = 0;
uint32_t g_outRightOffset = 0;
uint32_t g_outUpOffset = 0;
uint32_t g_outDownOffset = 0;

const uint32_t TEST_VPC_NUM = 1;
const uint32_t TEST_VPC_1_NUM = 2;
const uint32_t TEST_VDEC_NUM = 3;
const uint32_t TEST_VENC_NUM = 4;
const uint32_t TEST_JPEGE_NUM = 5;
const uint32_t TEST_JPEGE_CUSTOM_MEMORY_NUM = 6;
const uint32_t TEST_JPEGD_NUM = 7;
const uint32_t TEST_JPEGD_CUSTOM_MEMORY_NUM = 8;
const uint32_t TEST_PNGD_NUM = 9;
const uint32_t TEST_PNGD_CUSTOM_MEMORY_NUM = 10;
const uint32_t TEST_JPEGD_VPC_JPEGE_NUM = 11;
const uint32_t TEST_JPEGD_VPC_JPEGE_NEW_NUM = 12;
const uint32_t MIN_PARAM_NUM = 2;

const uint32_t PNGD_RGBA_FORMAT_NUM = 6;
const uint32_t PNGD_RGB_FORMAT_NUM = 2;

////////////////////////
uint64_t last_codec_ts[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
double g_delta_time[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int g_frame_cnt[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
uint64_t last_convert_ts[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
double g_delta_convert_time[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

////////////////


uint64_t Now() {
    auto now  = std::chrono::high_resolution_clock::now();
    auto nano_time_pt = std::chrono::time_point_cast<std::chrono::nanoseconds>(now);
    auto epoch = nano_time_pt.time_since_epoch();
    uint64_t now_nano = std::chrono::duration_cast<std::chrono::nanoseconds>(epoch).count();
    return now_nano;
}

void JpegeProcessBranch(IDVPPAPI*& pidvppapi, dvppapi_ctl_msg& dvppApiCtlMsg, sJpegeOut& outData);
void TEST_JPEGD_VPC_JPEGE_Branch(dvppapi_ctl_msg& vpcDvppApiCtlMsg, jpegd_yuv_data_info& jpegdOutData,
    vpc_in_msg& vpcInMsg);
int32_t FrameReturnSaveVpcResult(FRAME* frame, uint8_t* addr, int32_t imageCount);
void TestJpegdVpcJpegdNewBranch(uint8_t* vpcOutBuffer);

/*
 * common func, vdec callback function.
 */
void FrameReturn(FRAME* frame, void* hiaiData)
{
    int idx = 0;
    uint64_t now = Now();

    static int32_t imageCount = 0;
    imageCount++;
   
    uint64_t temp_delta = 0; 
    if (last_codec_ts[idx] == 0) {         
	last_codec_ts[idx] = now;
    } else {
        temp_delta = now - last_codec_ts[idx];
//	printf("%d-frame: %d, codec time: %f\n", idx, imageCount,  (now - last_codec_ts[idx]) / 1000000.0f);
        g_delta_time[idx] += (now - last_codec_ts[idx]);
        g_frame_cnt[idx]++;
        last_codec_ts[idx] = now;
    }

    //HIAI_ENGINE_LOG("call save frame number:[%d], width:[%d], height:[%d]", imageCount, frame->width, frame->height);
   // printf("0-frame number:[%d], width:[%d], height:[%d]\n", imageCount, frame->width, frame->height);;
    // The image directly output by vdec is an image of hfbc compression format, which cannot be directly displayed.
    // It is necessary to call vpc to convert hfbc to an image of uncompressed format to display.
    //HIAI_ENGINE_LOG("start call vpc interface to translate hfbc.");
    IDVPPAPI* dvppHandle = nullptr;
    int32_t ret = CreateDvppApi(dvppHandle);
    if (ret != 0) {
        HIAI_ENGINE_LOG(HIAI_CREATE_DVPP_ERROR, "creat dvpp api fail.");
        return;
    }

    // Construct vpc input configuration.
    std::shared_ptr<VpcUserImageConfigure> userImage(new VpcUserImageConfigure);
    // bareDataAddr should be null which the image is hfbc.
    userImage->bareDataAddr = nullptr;
    userImage->bareDataBufferSize = 0;
    userImage->widthStride = frame->width;
    userImage->heightStride = frame->height;
    // Configuration input format
    string imageFormat(frame->image_format);
    if (frame->bitdepth == 8) {
        if (imageFormat == "nv12") {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_UV;
        } else {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_VU;
        }
    } else {
        if (imageFormat == "nv12") {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_UV_10BIT;
        } else {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_VU_10BIT;
        }
    }
    userImage->outputFormat = OUTPUT_YUV420SP_UV;
    userImage->isCompressData = true;
    // Configure hfbc input address
    VpcCompressDataConfigure* compressDataConfigure = &userImage->compressDataConfigure;
    uint64_t baseAddr = (uint64_t)frame->buffer;
    compressDataConfigure->lumaHeadAddr = baseAddr + frame->offset_head_y;
    compressDataConfigure->chromaHeadAddr = baseAddr + frame->offset_head_c;
    compressDataConfigure->lumaPayloadAddr = baseAddr + frame->offset_payload_y;
    compressDataConfigure->chromaPayloadAddr = baseAddr + frame->offset_payload_c;
    compressDataConfigure->lumaHeadStride = frame->stride_head;
    compressDataConfigure->chromaHeadStride = frame->stride_head;
    compressDataConfigure->lumaPayloadStride = frame->stride_payload;
    compressDataConfigure->chromaPayloadStride = frame->stride_payload;

    userImage->yuvSumEnable = false;
    userImage->cmdListBufferAddr = nullptr;
    userImage->cmdListBufferSize = 0;
    // Configure the roi area and output area
    std::shared_ptr<VpcUserRoiConfigure> roiConfigure(new VpcUserRoiConfigure);
    roiConfigure->next = nullptr;
    userImage->roiConfigure = roiConfigure.get();
    VpcUserRoiInputConfigure* roiInput = &roiConfigure->inputConfigure;
    roiInput->cropArea.leftOffset = 0;
    roiInput->cropArea.rightOffset = frame->width - 1;
    roiInput->cropArea.upOffset = 0;
    roiInput->cropArea.downOffset = frame->height - 1;
    VpcUserRoiOutputConfigure* roiOutput = &roiConfigure->outputConfigure;
    roiOutput->outputArea.leftOffset = 0;
    roiOutput->outputArea.rightOffset = frame->width - 1;
    roiOutput->outputArea.upOffset = 0;
    roiOutput->outputArea.downOffset = frame->height - 1;
    roiOutput->bufferSize = ALIGN_UP(frame->width, 16) * ALIGN_UP(frame->height, 2) * 3 / 2;
    roiOutput->addr = (uint8_t*)HIAI_DVPP_DMalloc(roiOutput->bufferSize);
    roiOutput->widthStride = ALIGN_UP(frame->width, 16);
    roiOutput->heightStride = ALIGN_UP(frame->height, 2);

    dvppapi_ctl_msg dvppApiCtlMsg;
    dvppApiCtlMsg.in = (void*)(userImage.get());
    dvppApiCtlMsg.in_size = sizeof(VpcUserImageConfigure);
    ret = DvppCtl(dvppHandle, DVPP_CTL_VPC_PROC, &dvppApiCtlMsg);
    if (ret != 0) {
        HIAI_ENGINE_LOG(HIAI_ERROR, "call dvppctl fail");
        HIAI_DVPP_DFree(roiOutput->addr);
        DestroyDvppApi(dvppHandle);
        return;
    }
  //  ret = FrameReturnSaveVpcResult(frame, roiOutput->addr, imageCount);
  //  if (ret != 0) {
  //      HIAI_ENGINE_LOG(HIAI_ERROR, "save vpc result fail");
  //  }
    HIAI_DVPP_DFree(roiOutput->addr);
    DestroyDvppApi(dvppHandle);

    uint64_t now1 = Now(); 
    if (last_convert_ts[idx] == 0) {
        last_convert_ts[idx] = now1;
    } else {
        uint64_t delta = now1 - now;
        double delta_mill = delta / 1000000.0f;
        
  //      printf("%d-frame: %d, convert time: %f\n", idx, imageCount,  delta_mill + temp_delta / 1000000.0f);
        g_delta_convert_time[idx] += delta;
        g_delta_convert_time[idx] += temp_delta;
        last_convert_ts[idx] = now1;

    }

    return;
}



void FrameReturn1(FRAME* frame, void* hiaiData)
{
    int idx = 1;
    uint64_t now = Now();

    static int32_t imageCount = 0;
    imageCount++;
   
    uint64_t temp_delta = 0; 
    if (last_codec_ts[idx] == 0) {         
	last_codec_ts[idx] = now;
    } else {
        temp_delta = now - last_codec_ts[idx];
//	printf("%d-frame: %d, codec time: %f\n", idx, imageCount,  (now - last_codec_ts[idx]) / 1000000.0f);
        g_delta_time[idx] += (now - last_codec_ts[idx]);
        g_frame_cnt[idx]++;
        last_codec_ts[idx] = now;
    }

    //HIAI_ENGINE_LOG("call save frame number:[%d], width:[%d], height:[%d]", imageCount, frame->width, frame->height);
//    printf("0-frame number:[%d], width:[%d], height:[%d]\n", imageCount, frame->width, frame->height);;
    // The image directly output by vdec is an image of hfbc compression format, which cannot be directly displayed.
    // It is necessary to call vpc to convert hfbc to an image of uncompressed format to display.
    //HIAI_ENGINE_LOG("start call vpc interface to translate hfbc.");
    IDVPPAPI* dvppHandle = nullptr;
    int32_t ret = CreateDvppApi(dvppHandle);
    if (ret != 0) {
        HIAI_ENGINE_LOG(HIAI_CREATE_DVPP_ERROR, "creat dvpp api fail.");
        return;
    }

    // Construct vpc input configuration.
    std::shared_ptr<VpcUserImageConfigure> userImage(new VpcUserImageConfigure);
    // bareDataAddr should be null which the image is hfbc.
    userImage->bareDataAddr = nullptr;
    userImage->bareDataBufferSize = 0;
    userImage->widthStride = frame->width;
    userImage->heightStride = frame->height;
    // Configuration input format
    string imageFormat(frame->image_format);
    if (frame->bitdepth == 8) {
        if (imageFormat == "nv12") {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_UV;
        } else {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_VU;
        }
    } else {
        if (imageFormat == "nv12") {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_UV_10BIT;
        } else {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_VU_10BIT;
        }
    }
    userImage->outputFormat = OUTPUT_YUV420SP_UV;
    userImage->isCompressData = true;
    // Configure hfbc input address
    VpcCompressDataConfigure* compressDataConfigure = &userImage->compressDataConfigure;
    uint64_t baseAddr = (uint64_t)frame->buffer;
    compressDataConfigure->lumaHeadAddr = baseAddr + frame->offset_head_y;
    compressDataConfigure->chromaHeadAddr = baseAddr + frame->offset_head_c;
    compressDataConfigure->lumaPayloadAddr = baseAddr + frame->offset_payload_y;
    compressDataConfigure->chromaPayloadAddr = baseAddr + frame->offset_payload_c;
    compressDataConfigure->lumaHeadStride = frame->stride_head;
    compressDataConfigure->chromaHeadStride = frame->stride_head;
    compressDataConfigure->lumaPayloadStride = frame->stride_payload;
    compressDataConfigure->chromaPayloadStride = frame->stride_payload;

    userImage->yuvSumEnable = false;
    userImage->cmdListBufferAddr = nullptr;
    userImage->cmdListBufferSize = 0;
    // Configure the roi area and output area
    std::shared_ptr<VpcUserRoiConfigure> roiConfigure(new VpcUserRoiConfigure);
    roiConfigure->next = nullptr;
    userImage->roiConfigure = roiConfigure.get();
    VpcUserRoiInputConfigure* roiInput = &roiConfigure->inputConfigure;
    roiInput->cropArea.leftOffset = 0;
    roiInput->cropArea.rightOffset = frame->width - 1;
    roiInput->cropArea.upOffset = 0;
    roiInput->cropArea.downOffset = frame->height - 1;
    VpcUserRoiOutputConfigure* roiOutput = &roiConfigure->outputConfigure;
    roiOutput->outputArea.leftOffset = 0;
    roiOutput->outputArea.rightOffset = frame->width - 1;
    roiOutput->outputArea.upOffset = 0;
    roiOutput->outputArea.downOffset = frame->height - 1;
    roiOutput->bufferSize = ALIGN_UP(frame->width, 16) * ALIGN_UP(frame->height, 2) * 3 / 2;
    roiOutput->addr = (uint8_t*)HIAI_DVPP_DMalloc(roiOutput->bufferSize);
    roiOutput->widthStride = ALIGN_UP(frame->width, 16);
    roiOutput->heightStride = ALIGN_UP(frame->height, 2);

    dvppapi_ctl_msg dvppApiCtlMsg;
    dvppApiCtlMsg.in = (void*)(userImage.get());
    dvppApiCtlMsg.in_size = sizeof(VpcUserImageConfigure);
    ret = DvppCtl(dvppHandle, DVPP_CTL_VPC_PROC, &dvppApiCtlMsg);
    if (ret != 0) {
        HIAI_ENGINE_LOG(HIAI_ERROR, "call dvppctl fail");
        HIAI_DVPP_DFree(roiOutput->addr);
        DestroyDvppApi(dvppHandle);
        return;
    }
  //  ret = FrameReturnSaveVpcResult(frame, roiOutput->addr, imageCount);
  //  if (ret != 0) {
  //      HIAI_ENGINE_LOG(HIAI_ERROR, "save vpc result fail");
  //  }
    HIAI_DVPP_DFree(roiOutput->addr);
    DestroyDvppApi(dvppHandle);

    uint64_t now1 = Now(); 
    if (last_convert_ts[idx] == 0) {
        last_convert_ts[idx] = now1;
    } else {
        uint64_t delta = now1 - now;
        double delta_mill = delta / 1000000.0f;
        
//        printf("%d-frame: %d, convert time: %f\n", idx, imageCount,  delta_mill + temp_delta / 1000000.0f);
        g_delta_convert_time[idx] += delta;
        g_delta_convert_time[idx] += temp_delta;
        last_convert_ts[idx] = now1;

    }

    return;
}



void FrameReturn2(FRAME* frame, void* hiaiData)
{
    int idx = 2;
    uint64_t now = Now();

    static int32_t imageCount = 0;
    imageCount++;
   
    uint64_t temp_delta = 0; 
    if (last_codec_ts[idx] == 0) {         
	last_codec_ts[idx] = now;
    } else {
        temp_delta = now - last_codec_ts[idx];
//	printf("%d-frame: %d, codec time: %f\n", idx, imageCount,  (now - last_codec_ts[idx]) / 1000000.0f);
        g_delta_time[idx] += (now - last_codec_ts[idx]);
        g_frame_cnt[idx]++;
        last_codec_ts[idx] = now;
    }

    //HIAI_ENGINE_LOG("call save frame number:[%d], width:[%d], height:[%d]", imageCount, frame->width, frame->height);
//    printf("0-frame number:[%d], width:[%d], height:[%d]\n", imageCount, frame->width, frame->height);;
    // The image directly output by vdec is an image of hfbc compression format, which cannot be directly displayed.
    // It is necessary to call vpc to convert hfbc to an image of uncompressed format to display.
    //HIAI_ENGINE_LOG("start call vpc interface to translate hfbc.");
    IDVPPAPI* dvppHandle = nullptr;
    int32_t ret = CreateDvppApi(dvppHandle);
    if (ret != 0) {
        HIAI_ENGINE_LOG(HIAI_CREATE_DVPP_ERROR, "creat dvpp api fail.");
        return;
    }

    // Construct vpc input configuration.
    std::shared_ptr<VpcUserImageConfigure> userImage(new VpcUserImageConfigure);
    // bareDataAddr should be null which the image is hfbc.
    userImage->bareDataAddr = nullptr;
    userImage->bareDataBufferSize = 0;
    userImage->widthStride = frame->width;
    userImage->heightStride = frame->height;
    // Configuration input format
    string imageFormat(frame->image_format);
    if (frame->bitdepth == 8) {
        if (imageFormat == "nv12") {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_UV;
        } else {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_VU;
        }
    } else {
        if (imageFormat == "nv12") {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_UV_10BIT;
        } else {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_VU_10BIT;
        }
    }
    userImage->outputFormat = OUTPUT_YUV420SP_UV;
    userImage->isCompressData = true;
    // Configure hfbc input address
    VpcCompressDataConfigure* compressDataConfigure = &userImage->compressDataConfigure;
    uint64_t baseAddr = (uint64_t)frame->buffer;
    compressDataConfigure->lumaHeadAddr = baseAddr + frame->offset_head_y;
    compressDataConfigure->chromaHeadAddr = baseAddr + frame->offset_head_c;
    compressDataConfigure->lumaPayloadAddr = baseAddr + frame->offset_payload_y;
    compressDataConfigure->chromaPayloadAddr = baseAddr + frame->offset_payload_c;
    compressDataConfigure->lumaHeadStride = frame->stride_head;
    compressDataConfigure->chromaHeadStride = frame->stride_head;
    compressDataConfigure->lumaPayloadStride = frame->stride_payload;
    compressDataConfigure->chromaPayloadStride = frame->stride_payload;

    userImage->yuvSumEnable = false;
    userImage->cmdListBufferAddr = nullptr;
    userImage->cmdListBufferSize = 0;
    // Configure the roi area and output area
    std::shared_ptr<VpcUserRoiConfigure> roiConfigure(new VpcUserRoiConfigure);
    roiConfigure->next = nullptr;
    userImage->roiConfigure = roiConfigure.get();
    VpcUserRoiInputConfigure* roiInput = &roiConfigure->inputConfigure;
    roiInput->cropArea.leftOffset = 0;
    roiInput->cropArea.rightOffset = frame->width - 1;
    roiInput->cropArea.upOffset = 0;
    roiInput->cropArea.downOffset = frame->height - 1;
    VpcUserRoiOutputConfigure* roiOutput = &roiConfigure->outputConfigure;
    roiOutput->outputArea.leftOffset = 0;
    roiOutput->outputArea.rightOffset = frame->width - 1;
    roiOutput->outputArea.upOffset = 0;
    roiOutput->outputArea.downOffset = frame->height - 1;
    roiOutput->bufferSize = ALIGN_UP(frame->width, 16) * ALIGN_UP(frame->height, 2) * 3 / 2;
    roiOutput->addr = (uint8_t*)HIAI_DVPP_DMalloc(roiOutput->bufferSize);
    roiOutput->widthStride = ALIGN_UP(frame->width, 16);
    roiOutput->heightStride = ALIGN_UP(frame->height, 2);

    dvppapi_ctl_msg dvppApiCtlMsg;
    dvppApiCtlMsg.in = (void*)(userImage.get());
    dvppApiCtlMsg.in_size = sizeof(VpcUserImageConfigure);
    ret = DvppCtl(dvppHandle, DVPP_CTL_VPC_PROC, &dvppApiCtlMsg);
    if (ret != 0) {
        HIAI_ENGINE_LOG(HIAI_ERROR, "call dvppctl fail");
        HIAI_DVPP_DFree(roiOutput->addr);
        DestroyDvppApi(dvppHandle);
        return;
    }
  //  ret = FrameReturnSaveVpcResult(frame, roiOutput->addr, imageCount);
  //  if (ret != 0) {
  //      HIAI_ENGINE_LOG(HIAI_ERROR, "save vpc result fail");
  //  }
    HIAI_DVPP_DFree(roiOutput->addr);
    DestroyDvppApi(dvppHandle);

    uint64_t now1 = Now(); 
    if (last_convert_ts[idx] == 0) {
        last_convert_ts[idx] = now1;
    } else {
        uint64_t delta = now1 - now;
        double delta_mill = delta / 1000000.0f;
        
//        printf("%d-frame: %d, convert time: %f\n", idx, imageCount,  delta_mill + temp_delta / 1000000.0f);
        g_delta_convert_time[idx] += delta;
        g_delta_convert_time[idx] += temp_delta;
        last_convert_ts[idx] = now1;

    }

    return;
}


void FrameReturn3(FRAME* frame, void* hiaiData)
{
    int idx = 3;
    uint64_t now = Now();

    static int32_t imageCount = 0;
    imageCount++;
   
    uint64_t temp_delta = 0; 
    if (last_codec_ts[idx] == 0) {         
	last_codec_ts[idx] = now;
    } else {
        temp_delta = now - last_codec_ts[idx];
//	printf("%d-frame: %d, codec time: %f\n", idx, imageCount,  (now - last_codec_ts[idx]) / 1000000.0f);
        g_delta_time[idx] += (now - last_codec_ts[idx]);
        g_frame_cnt[idx]++;
        last_codec_ts[idx] = now;
    }

    //HIAI_ENGINE_LOG("call save frame number:[%d], width:[%d], height:[%d]", imageCount, frame->width, frame->height);
//    printf("0-frame number:[%d], width:[%d], height:[%d]\n", imageCount, frame->width, frame->height);;
    // The image directly output by vdec is an image of hfbc compression format, which cannot be directly displayed.
    // It is necessary to call vpc to convert hfbc to an image of uncompressed format to display.
    //HIAI_ENGINE_LOG("start call vpc interface to translate hfbc.");
    IDVPPAPI* dvppHandle = nullptr;
    int32_t ret = CreateDvppApi(dvppHandle);
    if (ret != 0) {
        HIAI_ENGINE_LOG(HIAI_CREATE_DVPP_ERROR, "creat dvpp api fail.");
        return;
    }

    // Construct vpc input configuration.
    std::shared_ptr<VpcUserImageConfigure> userImage(new VpcUserImageConfigure);
    // bareDataAddr should be null which the image is hfbc.
    userImage->bareDataAddr = nullptr;
    userImage->bareDataBufferSize = 0;
    userImage->widthStride = frame->width;
    userImage->heightStride = frame->height;
    // Configuration input format
    string imageFormat(frame->image_format);
    if (frame->bitdepth == 8) {
        if (imageFormat == "nv12") {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_UV;
        } else {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_VU;
        }
    } else {
        if (imageFormat == "nv12") {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_UV_10BIT;
        } else {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_VU_10BIT;
        }
    }
    userImage->outputFormat = OUTPUT_YUV420SP_UV;
    userImage->isCompressData = true;
    // Configure hfbc input address
    VpcCompressDataConfigure* compressDataConfigure = &userImage->compressDataConfigure;
    uint64_t baseAddr = (uint64_t)frame->buffer;
    compressDataConfigure->lumaHeadAddr = baseAddr + frame->offset_head_y;
    compressDataConfigure->chromaHeadAddr = baseAddr + frame->offset_head_c;
    compressDataConfigure->lumaPayloadAddr = baseAddr + frame->offset_payload_y;
    compressDataConfigure->chromaPayloadAddr = baseAddr + frame->offset_payload_c;
    compressDataConfigure->lumaHeadStride = frame->stride_head;
    compressDataConfigure->chromaHeadStride = frame->stride_head;
    compressDataConfigure->lumaPayloadStride = frame->stride_payload;
    compressDataConfigure->chromaPayloadStride = frame->stride_payload;

    userImage->yuvSumEnable = false;
    userImage->cmdListBufferAddr = nullptr;
    userImage->cmdListBufferSize = 0;
    // Configure the roi area and output area
    std::shared_ptr<VpcUserRoiConfigure> roiConfigure(new VpcUserRoiConfigure);
    roiConfigure->next = nullptr;
    userImage->roiConfigure = roiConfigure.get();
    VpcUserRoiInputConfigure* roiInput = &roiConfigure->inputConfigure;
    roiInput->cropArea.leftOffset = 0;
    roiInput->cropArea.rightOffset = frame->width - 1;
    roiInput->cropArea.upOffset = 0;
    roiInput->cropArea.downOffset = frame->height - 1;
    VpcUserRoiOutputConfigure* roiOutput = &roiConfigure->outputConfigure;
    roiOutput->outputArea.leftOffset = 0;
    roiOutput->outputArea.rightOffset = frame->width - 1;
    roiOutput->outputArea.upOffset = 0;
    roiOutput->outputArea.downOffset = frame->height - 1;
    roiOutput->bufferSize = ALIGN_UP(frame->width, 16) * ALIGN_UP(frame->height, 2) * 3 / 2;
    roiOutput->addr = (uint8_t*)HIAI_DVPP_DMalloc(roiOutput->bufferSize);
    roiOutput->widthStride = ALIGN_UP(frame->width, 16);
    roiOutput->heightStride = ALIGN_UP(frame->height, 2);

    dvppapi_ctl_msg dvppApiCtlMsg;
    dvppApiCtlMsg.in = (void*)(userImage.get());
    dvppApiCtlMsg.in_size = sizeof(VpcUserImageConfigure);
    ret = DvppCtl(dvppHandle, DVPP_CTL_VPC_PROC, &dvppApiCtlMsg);
    if (ret != 0) {
        HIAI_ENGINE_LOG(HIAI_ERROR, "call dvppctl fail");
        HIAI_DVPP_DFree(roiOutput->addr);
        DestroyDvppApi(dvppHandle);
        return;
    }
  //  ret = FrameReturnSaveVpcResult(frame, roiOutput->addr, imageCount);
  //  if (ret != 0) {
  //      HIAI_ENGINE_LOG(HIAI_ERROR, "save vpc result fail");
  //  }
    HIAI_DVPP_DFree(roiOutput->addr);
    DestroyDvppApi(dvppHandle);

    uint64_t now1 = Now(); 
    if (last_convert_ts[idx] == 0) {
        last_convert_ts[idx] = now1;
    } else {
        uint64_t delta = now1 - now;
        double delta_mill = delta / 1000000.0f;
        
//        printf("%d-frame: %d, convert time: %f\n", idx, imageCount,  delta_mill + temp_delta / 1000000.0f);
        g_delta_convert_time[idx] += delta;
        g_delta_convert_time[idx] += temp_delta;
        last_convert_ts[idx] = now1;

    }

    return;
}


void FrameReturn4(FRAME* frame, void* hiaiData)
{
    int idx = 4;
    uint64_t now = Now();

    static int32_t imageCount = 0;
    imageCount++;
   
    uint64_t temp_delta = 0; 
    if (last_codec_ts[idx] == 0) {         
	last_codec_ts[idx] = now;
    } else {
        temp_delta = now - last_codec_ts[idx];
//	printf("%d-frame: %d, codec time: %f\n", idx, imageCount,  (now - last_codec_ts[idx]) / 1000000.0f);
        g_delta_time[idx] += (now - last_codec_ts[idx]);
        g_frame_cnt[idx]++;
        last_codec_ts[idx] = now;
    }

    //HIAI_ENGINE_LOG("call save frame number:[%d], width:[%d], height:[%d]", imageCount, frame->width, frame->height);
//    printf("0-frame number:[%d], width:[%d], height:[%d]\n", imageCount, frame->width, frame->height);;
    // The image directly output by vdec is an image of hfbc compression format, which cannot be directly displayed.
    // It is necessary to call vpc to convert hfbc to an image of uncompressed format to display.
    //HIAI_ENGINE_LOG("start call vpc interface to translate hfbc.");
    IDVPPAPI* dvppHandle = nullptr;
    int32_t ret = CreateDvppApi(dvppHandle);
    if (ret != 0) {
        HIAI_ENGINE_LOG(HIAI_CREATE_DVPP_ERROR, "creat dvpp api fail.");
        return;
    }

    // Construct vpc input configuration.
    std::shared_ptr<VpcUserImageConfigure> userImage(new VpcUserImageConfigure);
    // bareDataAddr should be null which the image is hfbc.
    userImage->bareDataAddr = nullptr;
    userImage->bareDataBufferSize = 0;
    userImage->widthStride = frame->width;
    userImage->heightStride = frame->height;
    // Configuration input format
    string imageFormat(frame->image_format);
    if (frame->bitdepth == 8) {
        if (imageFormat == "nv12") {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_UV;
        } else {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_VU;
        }
    } else {
        if (imageFormat == "nv12") {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_UV_10BIT;
        } else {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_VU_10BIT;
        }
    }
    userImage->outputFormat = OUTPUT_YUV420SP_UV;
    userImage->isCompressData = true;
    // Configure hfbc input address
    VpcCompressDataConfigure* compressDataConfigure = &userImage->compressDataConfigure;
    uint64_t baseAddr = (uint64_t)frame->buffer;
    compressDataConfigure->lumaHeadAddr = baseAddr + frame->offset_head_y;
    compressDataConfigure->chromaHeadAddr = baseAddr + frame->offset_head_c;
    compressDataConfigure->lumaPayloadAddr = baseAddr + frame->offset_payload_y;
    compressDataConfigure->chromaPayloadAddr = baseAddr + frame->offset_payload_c;
    compressDataConfigure->lumaHeadStride = frame->stride_head;
    compressDataConfigure->chromaHeadStride = frame->stride_head;
    compressDataConfigure->lumaPayloadStride = frame->stride_payload;
    compressDataConfigure->chromaPayloadStride = frame->stride_payload;

    userImage->yuvSumEnable = false;
    userImage->cmdListBufferAddr = nullptr;
    userImage->cmdListBufferSize = 0;
    // Configure the roi area and output area
    std::shared_ptr<VpcUserRoiConfigure> roiConfigure(new VpcUserRoiConfigure);
    roiConfigure->next = nullptr;
    userImage->roiConfigure = roiConfigure.get();
    VpcUserRoiInputConfigure* roiInput = &roiConfigure->inputConfigure;
    roiInput->cropArea.leftOffset = 0;
    roiInput->cropArea.rightOffset = frame->width - 1;
    roiInput->cropArea.upOffset = 0;
    roiInput->cropArea.downOffset = frame->height - 1;
    VpcUserRoiOutputConfigure* roiOutput = &roiConfigure->outputConfigure;
    roiOutput->outputArea.leftOffset = 0;
    roiOutput->outputArea.rightOffset = frame->width - 1;
    roiOutput->outputArea.upOffset = 0;
    roiOutput->outputArea.downOffset = frame->height - 1;
    roiOutput->bufferSize = ALIGN_UP(frame->width, 16) * ALIGN_UP(frame->height, 2) * 3 / 2;
    roiOutput->addr = (uint8_t*)HIAI_DVPP_DMalloc(roiOutput->bufferSize);
    roiOutput->widthStride = ALIGN_UP(frame->width, 16);
    roiOutput->heightStride = ALIGN_UP(frame->height, 2);

    dvppapi_ctl_msg dvppApiCtlMsg;
    dvppApiCtlMsg.in = (void*)(userImage.get());
    dvppApiCtlMsg.in_size = sizeof(VpcUserImageConfigure);
    ret = DvppCtl(dvppHandle, DVPP_CTL_VPC_PROC, &dvppApiCtlMsg);
    if (ret != 0) {
        HIAI_ENGINE_LOG(HIAI_ERROR, "call dvppctl fail");
        HIAI_DVPP_DFree(roiOutput->addr);
        DestroyDvppApi(dvppHandle);
        return;
    }
  //  ret = FrameReturnSaveVpcResult(frame, roiOutput->addr, imageCount);
  //  if (ret != 0) {
  //      HIAI_ENGINE_LOG(HIAI_ERROR, "save vpc result fail");
  //  }
    HIAI_DVPP_DFree(roiOutput->addr);
    DestroyDvppApi(dvppHandle);

    uint64_t now1 = Now(); 
    if (last_convert_ts[idx] == 0) {
        last_convert_ts[idx] = now1;
    } else {
        uint64_t delta = now1 - now;
        double delta_mill = delta / 1000000.0f;
        
//        printf("%d-frame: %d, convert time: %f\n", idx, imageCount,  delta_mill + temp_delta / 1000000.0f);
        g_delta_convert_time[idx] += delta;
        g_delta_convert_time[idx] += temp_delta;
        last_convert_ts[idx] = now1;

    }

    return;
}


void FrameReturn5(FRAME* frame, void* hiaiData)
{
    int idx = 5;
    uint64_t now = Now();

    static int32_t imageCount = 0;
    imageCount++;
   
    uint64_t temp_delta = 0; 
    if (last_codec_ts[idx] == 0) {         
	last_codec_ts[idx] = now;
    } else {
        temp_delta = now - last_codec_ts[idx];
//	printf("%d-frame: %d, codec time: %f\n", idx, imageCount,  (now - last_codec_ts[idx]) / 1000000.0f);
        g_delta_time[idx] += (now - last_codec_ts[idx]);
        g_frame_cnt[idx]++;
        last_codec_ts[idx] = now;
    }

    //HIAI_ENGINE_LOG("call save frame number:[%d], width:[%d], height:[%d]", imageCount, frame->width, frame->height);
//    printf("0-frame number:[%d], width:[%d], height:[%d]\n", imageCount, frame->width, frame->height);;
    // The image directly output by vdec is an image of hfbc compression format, which cannot be directly displayed.
    // It is necessary to call vpc to convert hfbc to an image of uncompressed format to display.
    //HIAI_ENGINE_LOG("start call vpc interface to translate hfbc.");
    IDVPPAPI* dvppHandle = nullptr;
    int32_t ret = CreateDvppApi(dvppHandle);
    if (ret != 0) {
        HIAI_ENGINE_LOG(HIAI_CREATE_DVPP_ERROR, "creat dvpp api fail.");
        return;
    }

    // Construct vpc input configuration.
    std::shared_ptr<VpcUserImageConfigure> userImage(new VpcUserImageConfigure);
    // bareDataAddr should be null which the image is hfbc.
    userImage->bareDataAddr = nullptr;
    userImage->bareDataBufferSize = 0;
    userImage->widthStride = frame->width;
    userImage->heightStride = frame->height;
    // Configuration input format
    string imageFormat(frame->image_format);
    if (frame->bitdepth == 8) {
        if (imageFormat == "nv12") {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_UV;
        } else {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_VU;
        }
    } else {
        if (imageFormat == "nv12") {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_UV_10BIT;
        } else {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_VU_10BIT;
        }
    }
    userImage->outputFormat = OUTPUT_YUV420SP_UV;
    userImage->isCompressData = true;
    // Configure hfbc input address
    VpcCompressDataConfigure* compressDataConfigure = &userImage->compressDataConfigure;
    uint64_t baseAddr = (uint64_t)frame->buffer;
    compressDataConfigure->lumaHeadAddr = baseAddr + frame->offset_head_y;
    compressDataConfigure->chromaHeadAddr = baseAddr + frame->offset_head_c;
    compressDataConfigure->lumaPayloadAddr = baseAddr + frame->offset_payload_y;
    compressDataConfigure->chromaPayloadAddr = baseAddr + frame->offset_payload_c;
    compressDataConfigure->lumaHeadStride = frame->stride_head;
    compressDataConfigure->chromaHeadStride = frame->stride_head;
    compressDataConfigure->lumaPayloadStride = frame->stride_payload;
    compressDataConfigure->chromaPayloadStride = frame->stride_payload;

    userImage->yuvSumEnable = false;
    userImage->cmdListBufferAddr = nullptr;
    userImage->cmdListBufferSize = 0;
    // Configure the roi area and output area
    std::shared_ptr<VpcUserRoiConfigure> roiConfigure(new VpcUserRoiConfigure);
    roiConfigure->next = nullptr;
    userImage->roiConfigure = roiConfigure.get();
    VpcUserRoiInputConfigure* roiInput = &roiConfigure->inputConfigure;
    roiInput->cropArea.leftOffset = 0;
    roiInput->cropArea.rightOffset = frame->width - 1;
    roiInput->cropArea.upOffset = 0;
    roiInput->cropArea.downOffset = frame->height - 1;
    VpcUserRoiOutputConfigure* roiOutput = &roiConfigure->outputConfigure;
    roiOutput->outputArea.leftOffset = 0;
    roiOutput->outputArea.rightOffset = frame->width - 1;
    roiOutput->outputArea.upOffset = 0;
    roiOutput->outputArea.downOffset = frame->height - 1;
    roiOutput->bufferSize = ALIGN_UP(frame->width, 16) * ALIGN_UP(frame->height, 2) * 3 / 2;
    roiOutput->addr = (uint8_t*)HIAI_DVPP_DMalloc(roiOutput->bufferSize);
    roiOutput->widthStride = ALIGN_UP(frame->width, 16);
    roiOutput->heightStride = ALIGN_UP(frame->height, 2);

    dvppapi_ctl_msg dvppApiCtlMsg;
    dvppApiCtlMsg.in = (void*)(userImage.get());
    dvppApiCtlMsg.in_size = sizeof(VpcUserImageConfigure);
    ret = DvppCtl(dvppHandle, DVPP_CTL_VPC_PROC, &dvppApiCtlMsg);
    if (ret != 0) {
        HIAI_ENGINE_LOG(HIAI_ERROR, "call dvppctl fail");
        HIAI_DVPP_DFree(roiOutput->addr);
        DestroyDvppApi(dvppHandle);
        return;
    }
  //  ret = FrameReturnSaveVpcResult(frame, roiOutput->addr, imageCount);
  //  if (ret != 0) {
  //      HIAI_ENGINE_LOG(HIAI_ERROR, "save vpc result fail");
  //  }
    HIAI_DVPP_DFree(roiOutput->addr);
    DestroyDvppApi(dvppHandle);

    uint64_t now1 = Now(); 
    if (last_convert_ts[idx] == 0) {
        last_convert_ts[idx] = now1;
    } else {
        uint64_t delta = now1 - now;
        double delta_mill = delta / 1000000.0f;
        
//        printf("%d-frame: %d, convert time: %f\n", idx, imageCount,  delta_mill + temp_delta / 1000000.0f);
        g_delta_convert_time[idx] += delta;
        g_delta_convert_time[idx] += temp_delta;
        last_convert_ts[idx] = now1;

    }

    return;
}


void FrameReturn6(FRAME* frame, void* hiaiData)
{
    int idx = 6;
    uint64_t now = Now();

    static int32_t imageCount = 0;
    imageCount++;
   
    uint64_t temp_delta = 0; 
    if (last_codec_ts[idx] == 0) {         
	last_codec_ts[idx] = now;
    } else {
        temp_delta = now - last_codec_ts[idx];
//	printf("%d-frame: %d, codec time: %f\n", idx, imageCount,  (now - last_codec_ts[idx]) / 1000000.0f);
        g_delta_time[idx] += (now - last_codec_ts[idx]);
        g_frame_cnt[idx]++;
        last_codec_ts[idx] = now;
    }

    //HIAI_ENGINE_LOG("call save frame number:[%d], width:[%d], height:[%d]", imageCount, frame->width, frame->height);
//    printf("0-frame number:[%d], width:[%d], height:[%d]\n", imageCount, frame->width, frame->height);;
    // The image directly output by vdec is an image of hfbc compression format, which cannot be directly displayed.
    // It is necessary to call vpc to convert hfbc to an image of uncompressed format to display.
    //HIAI_ENGINE_LOG("start call vpc interface to translate hfbc.");
    IDVPPAPI* dvppHandle = nullptr;
    int32_t ret = CreateDvppApi(dvppHandle);
    if (ret != 0) {
        HIAI_ENGINE_LOG(HIAI_CREATE_DVPP_ERROR, "creat dvpp api fail.");
        return;
    }

    // Construct vpc input configuration.
    std::shared_ptr<VpcUserImageConfigure> userImage(new VpcUserImageConfigure);
    // bareDataAddr should be null which the image is hfbc.
    userImage->bareDataAddr = nullptr;
    userImage->bareDataBufferSize = 0;
    userImage->widthStride = frame->width;
    userImage->heightStride = frame->height;
    // Configuration input format
    string imageFormat(frame->image_format);
    if (frame->bitdepth == 8) {
        if (imageFormat == "nv12") {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_UV;
        } else {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_VU;
        }
    } else {
        if (imageFormat == "nv12") {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_UV_10BIT;
        } else {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_VU_10BIT;
        }
    }
    userImage->outputFormat = OUTPUT_YUV420SP_UV;
    userImage->isCompressData = true;
    // Configure hfbc input address
    VpcCompressDataConfigure* compressDataConfigure = &userImage->compressDataConfigure;
    uint64_t baseAddr = (uint64_t)frame->buffer;
    compressDataConfigure->lumaHeadAddr = baseAddr + frame->offset_head_y;
    compressDataConfigure->chromaHeadAddr = baseAddr + frame->offset_head_c;
    compressDataConfigure->lumaPayloadAddr = baseAddr + frame->offset_payload_y;
    compressDataConfigure->chromaPayloadAddr = baseAddr + frame->offset_payload_c;
    compressDataConfigure->lumaHeadStride = frame->stride_head;
    compressDataConfigure->chromaHeadStride = frame->stride_head;
    compressDataConfigure->lumaPayloadStride = frame->stride_payload;
    compressDataConfigure->chromaPayloadStride = frame->stride_payload;

    userImage->yuvSumEnable = false;
    userImage->cmdListBufferAddr = nullptr;
    userImage->cmdListBufferSize = 0;
    // Configure the roi area and output area
    std::shared_ptr<VpcUserRoiConfigure> roiConfigure(new VpcUserRoiConfigure);
    roiConfigure->next = nullptr;
    userImage->roiConfigure = roiConfigure.get();
    VpcUserRoiInputConfigure* roiInput = &roiConfigure->inputConfigure;
    roiInput->cropArea.leftOffset = 0;
    roiInput->cropArea.rightOffset = frame->width - 1;
    roiInput->cropArea.upOffset = 0;
    roiInput->cropArea.downOffset = frame->height - 1;
    VpcUserRoiOutputConfigure* roiOutput = &roiConfigure->outputConfigure;
    roiOutput->outputArea.leftOffset = 0;
    roiOutput->outputArea.rightOffset = frame->width - 1;
    roiOutput->outputArea.upOffset = 0;
    roiOutput->outputArea.downOffset = frame->height - 1;
    roiOutput->bufferSize = ALIGN_UP(frame->width, 16) * ALIGN_UP(frame->height, 2) * 3 / 2;
    roiOutput->addr = (uint8_t*)HIAI_DVPP_DMalloc(roiOutput->bufferSize);
    roiOutput->widthStride = ALIGN_UP(frame->width, 16);
    roiOutput->heightStride = ALIGN_UP(frame->height, 2);

    dvppapi_ctl_msg dvppApiCtlMsg;
    dvppApiCtlMsg.in = (void*)(userImage.get());
    dvppApiCtlMsg.in_size = sizeof(VpcUserImageConfigure);
    ret = DvppCtl(dvppHandle, DVPP_CTL_VPC_PROC, &dvppApiCtlMsg);
    if (ret != 0) {
        HIAI_ENGINE_LOG(HIAI_ERROR, "call dvppctl fail");
        HIAI_DVPP_DFree(roiOutput->addr);
        DestroyDvppApi(dvppHandle);
        return;
    }
  //  ret = FrameReturnSaveVpcResult(frame, roiOutput->addr, imageCount);
  //  if (ret != 0) {
  //      HIAI_ENGINE_LOG(HIAI_ERROR, "save vpc result fail");
  //  }
    HIAI_DVPP_DFree(roiOutput->addr);
    DestroyDvppApi(dvppHandle);

    uint64_t now1 = Now(); 
    if (last_convert_ts[idx] == 0) {
        last_convert_ts[idx] = now1;
    } else {
        uint64_t delta = now1 - now;
        double delta_mill = delta / 1000000.0f;
        
//        printf("%d-frame: %d, convert time: %f\n", idx, imageCount,  delta_mill + temp_delta / 1000000.0f);
        g_delta_convert_time[idx] += delta;
        g_delta_convert_time[idx] += temp_delta;
        last_convert_ts[idx] = now1;

    }

    return;
}

void FrameReturn7(FRAME* frame, void* hiaiData)
{
    int idx = 7;
    uint64_t now = Now();

    static int32_t imageCount = 0;
    imageCount++;
   
    uint64_t temp_delta = 0; 
    if (last_codec_ts[idx] == 0) {         
	last_codec_ts[idx] = now;
    } else {
        temp_delta = now - last_codec_ts[idx];
//	printf("%d-frame: %d, codec time: %f\n", idx, imageCount,  (now - last_codec_ts[idx]) / 1000000.0f);
        g_delta_time[idx] += (now - last_codec_ts[idx]);
        g_frame_cnt[idx]++;
        last_codec_ts[idx] = now;
    }

    //HIAI_ENGINE_LOG("call save frame number:[%d], width:[%d], height:[%d]", imageCount, frame->width, frame->height);
//    printf("0-frame number:[%d], width:[%d], height:[%d]\n", imageCount, frame->width, frame->height);;
    // The image directly output by vdec is an image of hfbc compression format, which cannot be directly displayed.
    // It is necessary to call vpc to convert hfbc to an image of uncompressed format to display.
    //HIAI_ENGINE_LOG("start call vpc interface to translate hfbc.");
    IDVPPAPI* dvppHandle = nullptr;
    int32_t ret = CreateDvppApi(dvppHandle);
    if (ret != 0) {
        HIAI_ENGINE_LOG(HIAI_CREATE_DVPP_ERROR, "creat dvpp api fail.");
        return;
    }

    // Construct vpc input configuration.
    std::shared_ptr<VpcUserImageConfigure> userImage(new VpcUserImageConfigure);
    // bareDataAddr should be null which the image is hfbc.
    userImage->bareDataAddr = nullptr;
    userImage->bareDataBufferSize = 0;
    userImage->widthStride = frame->width;
    userImage->heightStride = frame->height;
    // Configuration input format
    string imageFormat(frame->image_format);
    if (frame->bitdepth == 8) {
        if (imageFormat == "nv12") {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_UV;
        } else {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_VU;
        }
    } else {
        if (imageFormat == "nv12") {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_UV_10BIT;
        } else {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_VU_10BIT;
        }
    }
    userImage->outputFormat = OUTPUT_YUV420SP_UV;
    userImage->isCompressData = true;
    // Configure hfbc input address
    VpcCompressDataConfigure* compressDataConfigure = &userImage->compressDataConfigure;
    uint64_t baseAddr = (uint64_t)frame->buffer;
    compressDataConfigure->lumaHeadAddr = baseAddr + frame->offset_head_y;
    compressDataConfigure->chromaHeadAddr = baseAddr + frame->offset_head_c;
    compressDataConfigure->lumaPayloadAddr = baseAddr + frame->offset_payload_y;
    compressDataConfigure->chromaPayloadAddr = baseAddr + frame->offset_payload_c;
    compressDataConfigure->lumaHeadStride = frame->stride_head;
    compressDataConfigure->chromaHeadStride = frame->stride_head;
    compressDataConfigure->lumaPayloadStride = frame->stride_payload;
    compressDataConfigure->chromaPayloadStride = frame->stride_payload;

    userImage->yuvSumEnable = false;
    userImage->cmdListBufferAddr = nullptr;
    userImage->cmdListBufferSize = 0;
    // Configure the roi area and output area
    std::shared_ptr<VpcUserRoiConfigure> roiConfigure(new VpcUserRoiConfigure);
    roiConfigure->next = nullptr;
    userImage->roiConfigure = roiConfigure.get();
    VpcUserRoiInputConfigure* roiInput = &roiConfigure->inputConfigure;
    roiInput->cropArea.leftOffset = 0;
    roiInput->cropArea.rightOffset = frame->width - 1;
    roiInput->cropArea.upOffset = 0;
    roiInput->cropArea.downOffset = frame->height - 1;
    VpcUserRoiOutputConfigure* roiOutput = &roiConfigure->outputConfigure;
    roiOutput->outputArea.leftOffset = 0;
    roiOutput->outputArea.rightOffset = frame->width - 1;
    roiOutput->outputArea.upOffset = 0;
    roiOutput->outputArea.downOffset = frame->height - 1;
    roiOutput->bufferSize = ALIGN_UP(frame->width, 16) * ALIGN_UP(frame->height, 2) * 3 / 2;
    roiOutput->addr = (uint8_t*)HIAI_DVPP_DMalloc(roiOutput->bufferSize);
    roiOutput->widthStride = ALIGN_UP(frame->width, 16);
    roiOutput->heightStride = ALIGN_UP(frame->height, 2);

    dvppapi_ctl_msg dvppApiCtlMsg;
    dvppApiCtlMsg.in = (void*)(userImage.get());
    dvppApiCtlMsg.in_size = sizeof(VpcUserImageConfigure);
    ret = DvppCtl(dvppHandle, DVPP_CTL_VPC_PROC, &dvppApiCtlMsg);
    if (ret != 0) {
        HIAI_ENGINE_LOG(HIAI_ERROR, "call dvppctl fail");
        HIAI_DVPP_DFree(roiOutput->addr);
        DestroyDvppApi(dvppHandle);
        return;
    }
  //  ret = FrameReturnSaveVpcResult(frame, roiOutput->addr, imageCount);
  //  if (ret != 0) {
  //      HIAI_ENGINE_LOG(HIAI_ERROR, "save vpc result fail");
  //  }
    HIAI_DVPP_DFree(roiOutput->addr);
    DestroyDvppApi(dvppHandle);

    uint64_t now1 = Now(); 
    if (last_convert_ts[idx] == 0) {
        last_convert_ts[idx] = now1;
    } else {
        uint64_t delta = now1 - now;
        double delta_mill = delta / 1000000.0f;
        
//        printf("%d-frame: %d, convert time: %f\n", idx, imageCount,  delta_mill + temp_delta / 1000000.0f);
        g_delta_convert_time[idx] += delta;
        g_delta_convert_time[idx] += temp_delta;
        last_convert_ts[idx] = now1;

    }

    return;
}


void FrameReturn8(FRAME* frame, void* hiaiData)
{
    int idx = 8;
    uint64_t now = Now();

    static int32_t imageCount = 0;
    imageCount++;
   
    uint64_t temp_delta = 0; 
    if (last_codec_ts[idx] == 0) {         
	last_codec_ts[idx] = now;
    } else {
        temp_delta = now - last_codec_ts[idx];
//	printf("%d-frame: %d, codec time: %f\n", idx, imageCount,  (now - last_codec_ts[idx]) / 1000000.0f);
        g_delta_time[idx] += (now - last_codec_ts[idx]);
        g_frame_cnt[idx]++;
        last_codec_ts[idx] = now;
    }

    //HIAI_ENGINE_LOG("call save frame number:[%d], width:[%d], height:[%d]", imageCount, frame->width, frame->height);
//    printf("0-frame number:[%d], width:[%d], height:[%d]\n", imageCount, frame->width, frame->height);;
    // The image directly output by vdec is an image of hfbc compression format, which cannot be directly displayed.
    // It is necessary to call vpc to convert hfbc to an image of uncompressed format to display.
    //HIAI_ENGINE_LOG("start call vpc interface to translate hfbc.");
    IDVPPAPI* dvppHandle = nullptr;
    int32_t ret = CreateDvppApi(dvppHandle);
    if (ret != 0) {
        HIAI_ENGINE_LOG(HIAI_CREATE_DVPP_ERROR, "creat dvpp api fail.");
        return;
    }

    // Construct vpc input configuration.
    std::shared_ptr<VpcUserImageConfigure> userImage(new VpcUserImageConfigure);
    // bareDataAddr should be null which the image is hfbc.
    userImage->bareDataAddr = nullptr;
    userImage->bareDataBufferSize = 0;
    userImage->widthStride = frame->width;
    userImage->heightStride = frame->height;
    // Configuration input format
    string imageFormat(frame->image_format);
    if (frame->bitdepth == 8) {
        if (imageFormat == "nv12") {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_UV;
        } else {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_VU;
        }
    } else {
        if (imageFormat == "nv12") {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_UV_10BIT;
        } else {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_VU_10BIT;
        }
    }
    userImage->outputFormat = OUTPUT_YUV420SP_UV;
    userImage->isCompressData = true;
    // Configure hfbc input address
    VpcCompressDataConfigure* compressDataConfigure = &userImage->compressDataConfigure;
    uint64_t baseAddr = (uint64_t)frame->buffer;
    compressDataConfigure->lumaHeadAddr = baseAddr + frame->offset_head_y;
    compressDataConfigure->chromaHeadAddr = baseAddr + frame->offset_head_c;
    compressDataConfigure->lumaPayloadAddr = baseAddr + frame->offset_payload_y;
    compressDataConfigure->chromaPayloadAddr = baseAddr + frame->offset_payload_c;
    compressDataConfigure->lumaHeadStride = frame->stride_head;
    compressDataConfigure->chromaHeadStride = frame->stride_head;
    compressDataConfigure->lumaPayloadStride = frame->stride_payload;
    compressDataConfigure->chromaPayloadStride = frame->stride_payload;

    userImage->yuvSumEnable = false;
    userImage->cmdListBufferAddr = nullptr;
    userImage->cmdListBufferSize = 0;
    // Configure the roi area and output area
    std::shared_ptr<VpcUserRoiConfigure> roiConfigure(new VpcUserRoiConfigure);
    roiConfigure->next = nullptr;
    userImage->roiConfigure = roiConfigure.get();
    VpcUserRoiInputConfigure* roiInput = &roiConfigure->inputConfigure;
    roiInput->cropArea.leftOffset = 0;
    roiInput->cropArea.rightOffset = frame->width - 1;
    roiInput->cropArea.upOffset = 0;
    roiInput->cropArea.downOffset = frame->height - 1;
    VpcUserRoiOutputConfigure* roiOutput = &roiConfigure->outputConfigure;
    roiOutput->outputArea.leftOffset = 0;
    roiOutput->outputArea.rightOffset = frame->width - 1;
    roiOutput->outputArea.upOffset = 0;
    roiOutput->outputArea.downOffset = frame->height - 1;
    roiOutput->bufferSize = ALIGN_UP(frame->width, 16) * ALIGN_UP(frame->height, 2) * 3 / 2;
    roiOutput->addr = (uint8_t*)HIAI_DVPP_DMalloc(roiOutput->bufferSize);
    roiOutput->widthStride = ALIGN_UP(frame->width, 16);
    roiOutput->heightStride = ALIGN_UP(frame->height, 2);

    dvppapi_ctl_msg dvppApiCtlMsg;
    dvppApiCtlMsg.in = (void*)(userImage.get());
    dvppApiCtlMsg.in_size = sizeof(VpcUserImageConfigure);
    ret = DvppCtl(dvppHandle, DVPP_CTL_VPC_PROC, &dvppApiCtlMsg);
    if (ret != 0) {
        HIAI_ENGINE_LOG(HIAI_ERROR, "call dvppctl fail");
        HIAI_DVPP_DFree(roiOutput->addr);
        DestroyDvppApi(dvppHandle);
        return;
    }
  //  ret = FrameReturnSaveVpcResult(frame, roiOutput->addr, imageCount);
  //  if (ret != 0) {
  //      HIAI_ENGINE_LOG(HIAI_ERROR, "save vpc result fail");
  //  }
    HIAI_DVPP_DFree(roiOutput->addr);
    DestroyDvppApi(dvppHandle);

    uint64_t now1 = Now(); 
    if (last_convert_ts[idx] == 0) {
        last_convert_ts[idx] = now1;
    } else {
        uint64_t delta = now1 - now;
        double delta_mill = delta / 1000000.0f;
        
//        printf("%d-frame: %d, convert time: %f\n", idx, imageCount,  delta_mill + temp_delta / 1000000.0f);
        g_delta_convert_time[idx] += delta;
        g_delta_convert_time[idx] += temp_delta;
        last_convert_ts[idx] = now1;

    }

    return;
}


void FrameReturn9(FRAME* frame, void* hiaiData)
{
    int idx = 9;
    uint64_t now = Now();

    static int32_t imageCount = 0;
    imageCount++;
   
    uint64_t temp_delta = 0; 
    if (last_codec_ts[idx] == 0) {         
	last_codec_ts[idx] = now;
    } else {
        temp_delta = now - last_codec_ts[idx];
//	printf("%d-frame: %d, codec time: %f\n", idx, imageCount,  (now - last_codec_ts[idx]) / 1000000.0f);
        g_delta_time[idx] += (now - last_codec_ts[idx]);
        g_frame_cnt[idx]++;
        last_codec_ts[idx] = now;
    }

    //HIAI_ENGINE_LOG("call save frame number:[%d], width:[%d], height:[%d]", imageCount, frame->width, frame->height);
//    printf("0-frame number:[%d], width:[%d], height:[%d]\n", imageCount, frame->width, frame->height);;
    // The image directly output by vdec is an image of hfbc compression format, which cannot be directly displayed.
    // It is necessary to call vpc to convert hfbc to an image of uncompressed format to display.
    //HIAI_ENGINE_LOG("start call vpc interface to translate hfbc.");
    IDVPPAPI* dvppHandle = nullptr;
    int32_t ret = CreateDvppApi(dvppHandle);
    if (ret != 0) {
        HIAI_ENGINE_LOG(HIAI_CREATE_DVPP_ERROR, "creat dvpp api fail.");
        return;
    }

    // Construct vpc input configuration.
    std::shared_ptr<VpcUserImageConfigure> userImage(new VpcUserImageConfigure);
    // bareDataAddr should be null which the image is hfbc.
    userImage->bareDataAddr = nullptr;
    userImage->bareDataBufferSize = 0;
    userImage->widthStride = frame->width;
    userImage->heightStride = frame->height;
    // Configuration input format
    string imageFormat(frame->image_format);
    if (frame->bitdepth == 8) {
        if (imageFormat == "nv12") {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_UV;
        } else {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_VU;
        }
    } else {
        if (imageFormat == "nv12") {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_UV_10BIT;
        } else {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_VU_10BIT;
        }
    }
    userImage->outputFormat = OUTPUT_YUV420SP_UV;
    userImage->isCompressData = true;
    // Configure hfbc input address
    VpcCompressDataConfigure* compressDataConfigure = &userImage->compressDataConfigure;
    uint64_t baseAddr = (uint64_t)frame->buffer;
    compressDataConfigure->lumaHeadAddr = baseAddr + frame->offset_head_y;
    compressDataConfigure->chromaHeadAddr = baseAddr + frame->offset_head_c;
    compressDataConfigure->lumaPayloadAddr = baseAddr + frame->offset_payload_y;
    compressDataConfigure->chromaPayloadAddr = baseAddr + frame->offset_payload_c;
    compressDataConfigure->lumaHeadStride = frame->stride_head;
    compressDataConfigure->chromaHeadStride = frame->stride_head;
    compressDataConfigure->lumaPayloadStride = frame->stride_payload;
    compressDataConfigure->chromaPayloadStride = frame->stride_payload;

    userImage->yuvSumEnable = false;
    userImage->cmdListBufferAddr = nullptr;
    userImage->cmdListBufferSize = 0;
    // Configure the roi area and output area
    std::shared_ptr<VpcUserRoiConfigure> roiConfigure(new VpcUserRoiConfigure);
    roiConfigure->next = nullptr;
    userImage->roiConfigure = roiConfigure.get();
    VpcUserRoiInputConfigure* roiInput = &roiConfigure->inputConfigure;
    roiInput->cropArea.leftOffset = 0;
    roiInput->cropArea.rightOffset = frame->width - 1;
    roiInput->cropArea.upOffset = 0;
    roiInput->cropArea.downOffset = frame->height - 1;
    VpcUserRoiOutputConfigure* roiOutput = &roiConfigure->outputConfigure;
    roiOutput->outputArea.leftOffset = 0;
    roiOutput->outputArea.rightOffset = frame->width - 1;
    roiOutput->outputArea.upOffset = 0;
    roiOutput->outputArea.downOffset = frame->height - 1;
    roiOutput->bufferSize = ALIGN_UP(frame->width, 16) * ALIGN_UP(frame->height, 2) * 3 / 2;
    roiOutput->addr = (uint8_t*)HIAI_DVPP_DMalloc(roiOutput->bufferSize);
    roiOutput->widthStride = ALIGN_UP(frame->width, 16);
    roiOutput->heightStride = ALIGN_UP(frame->height, 2);

    dvppapi_ctl_msg dvppApiCtlMsg;
    dvppApiCtlMsg.in = (void*)(userImage.get());
    dvppApiCtlMsg.in_size = sizeof(VpcUserImageConfigure);
    ret = DvppCtl(dvppHandle, DVPP_CTL_VPC_PROC, &dvppApiCtlMsg);
    if (ret != 0) {
        HIAI_ENGINE_LOG(HIAI_ERROR, "call dvppctl fail");
        HIAI_DVPP_DFree(roiOutput->addr);
        DestroyDvppApi(dvppHandle);
        return;
    }
  //  ret = FrameReturnSaveVpcResult(frame, roiOutput->addr, imageCount);
  //  if (ret != 0) {
  //      HIAI_ENGINE_LOG(HIAI_ERROR, "save vpc result fail");
  //  }
    HIAI_DVPP_DFree(roiOutput->addr);
    DestroyDvppApi(dvppHandle);

    uint64_t now1 = Now(); 
    if (last_convert_ts[idx] == 0) {
        last_convert_ts[idx] = now1;
    } else {
        uint64_t delta = now1 - now;
        double delta_mill = delta / 1000000.0f;
        
//        printf("%d-frame: %d, convert time: %f\n", idx, imageCount,  delta_mill + temp_delta / 1000000.0f);
        g_delta_convert_time[idx] += delta;
        g_delta_convert_time[idx] += temp_delta;
        last_convert_ts[idx] = now1;

    }

    return;
}

void FrameReturn10(FRAME* frame, void* hiaiData)
{
    int idx = 10;
    uint64_t now = Now();

    static int32_t imageCount = 0;
    imageCount++;
   
    uint64_t temp_delta = 0; 
    if (last_codec_ts[idx] == 0) {         
	last_codec_ts[idx] = now;
    } else {
        temp_delta = now - last_codec_ts[idx];
//	printf("%d-frame: %d, codec time: %f\n", idx, imageCount,  (now - last_codec_ts[idx]) / 1000000.0f);
        g_delta_time[idx] += (now - last_codec_ts[idx]);
        g_frame_cnt[idx]++;
        last_codec_ts[idx] = now;
    }

    //HIAI_ENGINE_LOG("call save frame number:[%d], width:[%d], height:[%d]", imageCount, frame->width, frame->height);
//    printf("0-frame number:[%d], width:[%d], height:[%d]\n", imageCount, frame->width, frame->height);;
    // The image directly output by vdec is an image of hfbc compression format, which cannot be directly displayed.
    // It is necessary to call vpc to convert hfbc to an image of uncompressed format to display.
    //HIAI_ENGINE_LOG("start call vpc interface to translate hfbc.");
    IDVPPAPI* dvppHandle = nullptr;
    int32_t ret = CreateDvppApi(dvppHandle);
    if (ret != 0) {
        HIAI_ENGINE_LOG(HIAI_CREATE_DVPP_ERROR, "creat dvpp api fail.");
        return;
    }

    // Construct vpc input configuration.
    std::shared_ptr<VpcUserImageConfigure> userImage(new VpcUserImageConfigure);
    // bareDataAddr should be null which the image is hfbc.
    userImage->bareDataAddr = nullptr;
    userImage->bareDataBufferSize = 0;
    userImage->widthStride = frame->width;
    userImage->heightStride = frame->height;
    // Configuration input format
    string imageFormat(frame->image_format);
    if (frame->bitdepth == 8) {
        if (imageFormat == "nv12") {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_UV;
        } else {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_VU;
        }
    } else {
        if (imageFormat == "nv12") {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_UV_10BIT;
        } else {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_VU_10BIT;
        }
    }
    userImage->outputFormat = OUTPUT_YUV420SP_UV;
    userImage->isCompressData = true;
    // Configure hfbc input address
    VpcCompressDataConfigure* compressDataConfigure = &userImage->compressDataConfigure;
    uint64_t baseAddr = (uint64_t)frame->buffer;
    compressDataConfigure->lumaHeadAddr = baseAddr + frame->offset_head_y;
    compressDataConfigure->chromaHeadAddr = baseAddr + frame->offset_head_c;
    compressDataConfigure->lumaPayloadAddr = baseAddr + frame->offset_payload_y;
    compressDataConfigure->chromaPayloadAddr = baseAddr + frame->offset_payload_c;
    compressDataConfigure->lumaHeadStride = frame->stride_head;
    compressDataConfigure->chromaHeadStride = frame->stride_head;
    compressDataConfigure->lumaPayloadStride = frame->stride_payload;
    compressDataConfigure->chromaPayloadStride = frame->stride_payload;

    userImage->yuvSumEnable = false;
    userImage->cmdListBufferAddr = nullptr;
    userImage->cmdListBufferSize = 0;
    // Configure the roi area and output area
    std::shared_ptr<VpcUserRoiConfigure> roiConfigure(new VpcUserRoiConfigure);
    roiConfigure->next = nullptr;
    userImage->roiConfigure = roiConfigure.get();
    VpcUserRoiInputConfigure* roiInput = &roiConfigure->inputConfigure;
    roiInput->cropArea.leftOffset = 0;
    roiInput->cropArea.rightOffset = frame->width - 1;
    roiInput->cropArea.upOffset = 0;
    roiInput->cropArea.downOffset = frame->height - 1;
    VpcUserRoiOutputConfigure* roiOutput = &roiConfigure->outputConfigure;
    roiOutput->outputArea.leftOffset = 0;
    roiOutput->outputArea.rightOffset = frame->width - 1;
    roiOutput->outputArea.upOffset = 0;
    roiOutput->outputArea.downOffset = frame->height - 1;
    roiOutput->bufferSize = ALIGN_UP(frame->width, 16) * ALIGN_UP(frame->height, 2) * 3 / 2;
    roiOutput->addr = (uint8_t*)HIAI_DVPP_DMalloc(roiOutput->bufferSize);
    roiOutput->widthStride = ALIGN_UP(frame->width, 16);
    roiOutput->heightStride = ALIGN_UP(frame->height, 2);

    dvppapi_ctl_msg dvppApiCtlMsg;
    dvppApiCtlMsg.in = (void*)(userImage.get());
    dvppApiCtlMsg.in_size = sizeof(VpcUserImageConfigure);
    ret = DvppCtl(dvppHandle, DVPP_CTL_VPC_PROC, &dvppApiCtlMsg);
    if (ret != 0) {
        HIAI_ENGINE_LOG(HIAI_ERROR, "call dvppctl fail");
        HIAI_DVPP_DFree(roiOutput->addr);
        DestroyDvppApi(dvppHandle);
        return;
    }
  //  ret = FrameReturnSaveVpcResult(frame, roiOutput->addr, imageCount);
  //  if (ret != 0) {
  //      HIAI_ENGINE_LOG(HIAI_ERROR, "save vpc result fail");
  //  }
    HIAI_DVPP_DFree(roiOutput->addr);
    DestroyDvppApi(dvppHandle);

    uint64_t now1 = Now(); 
    if (last_convert_ts[idx] == 0) {
        last_convert_ts[idx] = now1;
    } else {
        uint64_t delta = now1 - now;
        double delta_mill = delta / 1000000.0f;
        
//        printf("%d-frame: %d, convert time: %f\n", idx, imageCount,  delta_mill + temp_delta / 1000000.0f);
        g_delta_convert_time[idx] += delta;
        g_delta_convert_time[idx] += temp_delta;
        last_convert_ts[idx] = now1;

    }

    return;
}

void FrameReturn11(FRAME* frame, void* hiaiData)
{
    int idx = 11;
    uint64_t now = Now();

    static int32_t imageCount = 0;
    imageCount++;
   
    uint64_t temp_delta = 0; 
    if (last_codec_ts[idx] == 0) {         
	last_codec_ts[idx] = now;
    } else {
        temp_delta = now - last_codec_ts[idx];
//	printf("%d-frame: %d, codec time: %f\n", idx, imageCount,  (now - last_codec_ts[idx]) / 1000000.0f);
        g_delta_time[idx] += (now - last_codec_ts[idx]);
        g_frame_cnt[idx]++;
        last_codec_ts[idx] = now;
    }

    //HIAI_ENGINE_LOG("call save frame number:[%d], width:[%d], height:[%d]", imageCount, frame->width, frame->height);
//    printf("0-frame number:[%d], width:[%d], height:[%d]\n", imageCount, frame->width, frame->height);;
    // The image directly output by vdec is an image of hfbc compression format, which cannot be directly displayed.
    // It is necessary to call vpc to convert hfbc to an image of uncompressed format to display.
    //HIAI_ENGINE_LOG("start call vpc interface to translate hfbc.");
    IDVPPAPI* dvppHandle = nullptr;
    int32_t ret = CreateDvppApi(dvppHandle);
    if (ret != 0) {
        HIAI_ENGINE_LOG(HIAI_CREATE_DVPP_ERROR, "creat dvpp api fail.");
        return;
    }

    // Construct vpc input configuration.
    std::shared_ptr<VpcUserImageConfigure> userImage(new VpcUserImageConfigure);
    // bareDataAddr should be null which the image is hfbc.
    userImage->bareDataAddr = nullptr;
    userImage->bareDataBufferSize = 0;
    userImage->widthStride = frame->width;
    userImage->heightStride = frame->height;
    // Configuration input format
    string imageFormat(frame->image_format);
    if (frame->bitdepth == 8) {
        if (imageFormat == "nv12") {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_UV;
        } else {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_VU;
        }
    } else {
        if (imageFormat == "nv12") {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_UV_10BIT;
        } else {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_VU_10BIT;
        }
    }
    userImage->outputFormat = OUTPUT_YUV420SP_UV;
    userImage->isCompressData = true;
    // Configure hfbc input address
    VpcCompressDataConfigure* compressDataConfigure = &userImage->compressDataConfigure;
    uint64_t baseAddr = (uint64_t)frame->buffer;
    compressDataConfigure->lumaHeadAddr = baseAddr + frame->offset_head_y;
    compressDataConfigure->chromaHeadAddr = baseAddr + frame->offset_head_c;
    compressDataConfigure->lumaPayloadAddr = baseAddr + frame->offset_payload_y;
    compressDataConfigure->chromaPayloadAddr = baseAddr + frame->offset_payload_c;
    compressDataConfigure->lumaHeadStride = frame->stride_head;
    compressDataConfigure->chromaHeadStride = frame->stride_head;
    compressDataConfigure->lumaPayloadStride = frame->stride_payload;
    compressDataConfigure->chromaPayloadStride = frame->stride_payload;

    userImage->yuvSumEnable = false;
    userImage->cmdListBufferAddr = nullptr;
    userImage->cmdListBufferSize = 0;
    // Configure the roi area and output area
    std::shared_ptr<VpcUserRoiConfigure> roiConfigure(new VpcUserRoiConfigure);
    roiConfigure->next = nullptr;
    userImage->roiConfigure = roiConfigure.get();
    VpcUserRoiInputConfigure* roiInput = &roiConfigure->inputConfigure;
    roiInput->cropArea.leftOffset = 0;
    roiInput->cropArea.rightOffset = frame->width - 1;
    roiInput->cropArea.upOffset = 0;
    roiInput->cropArea.downOffset = frame->height - 1;
    VpcUserRoiOutputConfigure* roiOutput = &roiConfigure->outputConfigure;
    roiOutput->outputArea.leftOffset = 0;
    roiOutput->outputArea.rightOffset = frame->width - 1;
    roiOutput->outputArea.upOffset = 0;
    roiOutput->outputArea.downOffset = frame->height - 1;
    roiOutput->bufferSize = ALIGN_UP(frame->width, 16) * ALIGN_UP(frame->height, 2) * 3 / 2;
    roiOutput->addr = (uint8_t*)HIAI_DVPP_DMalloc(roiOutput->bufferSize);
    roiOutput->widthStride = ALIGN_UP(frame->width, 16);
    roiOutput->heightStride = ALIGN_UP(frame->height, 2);

    dvppapi_ctl_msg dvppApiCtlMsg;
    dvppApiCtlMsg.in = (void*)(userImage.get());
    dvppApiCtlMsg.in_size = sizeof(VpcUserImageConfigure);
    ret = DvppCtl(dvppHandle, DVPP_CTL_VPC_PROC, &dvppApiCtlMsg);
    if (ret != 0) {
        HIAI_ENGINE_LOG(HIAI_ERROR, "call dvppctl fail");
        HIAI_DVPP_DFree(roiOutput->addr);
        DestroyDvppApi(dvppHandle);
        return;
    }
  //  ret = FrameReturnSaveVpcResult(frame, roiOutput->addr, imageCount);
  //  if (ret != 0) {
  //      HIAI_ENGINE_LOG(HIAI_ERROR, "save vpc result fail");
  //  }
    HIAI_DVPP_DFree(roiOutput->addr);
    DestroyDvppApi(dvppHandle);

    uint64_t now1 = Now(); 
    if (last_convert_ts[idx] == 0) {
        last_convert_ts[idx] = now1;
    } else {
        uint64_t delta = now1 - now;
        double delta_mill = delta / 1000000.0f;
        
//        printf("%d-frame: %d, convert time: %f\n", idx, imageCount,  delta_mill + temp_delta / 1000000.0f);
        g_delta_convert_time[idx] += delta;
        g_delta_convert_time[idx] += temp_delta;
        last_convert_ts[idx] = now1;

    }

    return;
}

void FrameReturn12(FRAME* frame, void* hiaiData)
{
    int idx = 12;
    uint64_t now = Now();

    static int32_t imageCount = 0;
    imageCount++;
   
    uint64_t temp_delta = 0; 
    if (last_codec_ts[idx] == 0) {         
	last_codec_ts[idx] = now;
    } else {
        temp_delta = now - last_codec_ts[idx];
//	printf("%d-frame: %d, codec time: %f\n", idx, imageCount,  (now - last_codec_ts[idx]) / 1000000.0f);
        g_delta_time[idx] += (now - last_codec_ts[idx]);
        g_frame_cnt[idx]++;
        last_codec_ts[idx] = now;
    }

    //HIAI_ENGINE_LOG("call save frame number:[%d], width:[%d], height:[%d]", imageCount, frame->width, frame->height);
//    printf("0-frame number:[%d], width:[%d], height:[%d]\n", imageCount, frame->width, frame->height);;
    // The image directly output by vdec is an image of hfbc compression format, which cannot be directly displayed.
    // It is necessary to call vpc to convert hfbc to an image of uncompressed format to display.
    //HIAI_ENGINE_LOG("start call vpc interface to translate hfbc.");
    IDVPPAPI* dvppHandle = nullptr;
    int32_t ret = CreateDvppApi(dvppHandle);
    if (ret != 0) {
        HIAI_ENGINE_LOG(HIAI_CREATE_DVPP_ERROR, "creat dvpp api fail.");
        return;
    }

    // Construct vpc input configuration.
    std::shared_ptr<VpcUserImageConfigure> userImage(new VpcUserImageConfigure);
    // bareDataAddr should be null which the image is hfbc.
    userImage->bareDataAddr = nullptr;
    userImage->bareDataBufferSize = 0;
    userImage->widthStride = frame->width;
    userImage->heightStride = frame->height;
    // Configuration input format
    string imageFormat(frame->image_format);
    if (frame->bitdepth == 8) {
        if (imageFormat == "nv12") {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_UV;
        } else {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_VU;
        }
    } else {
        if (imageFormat == "nv12") {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_UV_10BIT;
        } else {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_VU_10BIT;
        }
    }
    userImage->outputFormat = OUTPUT_YUV420SP_UV;
    userImage->isCompressData = true;
    // Configure hfbc input address
    VpcCompressDataConfigure* compressDataConfigure = &userImage->compressDataConfigure;
    uint64_t baseAddr = (uint64_t)frame->buffer;
    compressDataConfigure->lumaHeadAddr = baseAddr + frame->offset_head_y;
    compressDataConfigure->chromaHeadAddr = baseAddr + frame->offset_head_c;
    compressDataConfigure->lumaPayloadAddr = baseAddr + frame->offset_payload_y;
    compressDataConfigure->chromaPayloadAddr = baseAddr + frame->offset_payload_c;
    compressDataConfigure->lumaHeadStride = frame->stride_head;
    compressDataConfigure->chromaHeadStride = frame->stride_head;
    compressDataConfigure->lumaPayloadStride = frame->stride_payload;
    compressDataConfigure->chromaPayloadStride = frame->stride_payload;

    userImage->yuvSumEnable = false;
    userImage->cmdListBufferAddr = nullptr;
    userImage->cmdListBufferSize = 0;
    // Configure the roi area and output area
    std::shared_ptr<VpcUserRoiConfigure> roiConfigure(new VpcUserRoiConfigure);
    roiConfigure->next = nullptr;
    userImage->roiConfigure = roiConfigure.get();
    VpcUserRoiInputConfigure* roiInput = &roiConfigure->inputConfigure;
    roiInput->cropArea.leftOffset = 0;
    roiInput->cropArea.rightOffset = frame->width - 1;
    roiInput->cropArea.upOffset = 0;
    roiInput->cropArea.downOffset = frame->height - 1;
    VpcUserRoiOutputConfigure* roiOutput = &roiConfigure->outputConfigure;
    roiOutput->outputArea.leftOffset = 0;
    roiOutput->outputArea.rightOffset = frame->width - 1;
    roiOutput->outputArea.upOffset = 0;
    roiOutput->outputArea.downOffset = frame->height - 1;
    roiOutput->bufferSize = ALIGN_UP(frame->width, 16) * ALIGN_UP(frame->height, 2) * 3 / 2;
    roiOutput->addr = (uint8_t*)HIAI_DVPP_DMalloc(roiOutput->bufferSize);
    roiOutput->widthStride = ALIGN_UP(frame->width, 16);
    roiOutput->heightStride = ALIGN_UP(frame->height, 2);

    dvppapi_ctl_msg dvppApiCtlMsg;
    dvppApiCtlMsg.in = (void*)(userImage.get());
    dvppApiCtlMsg.in_size = sizeof(VpcUserImageConfigure);
    ret = DvppCtl(dvppHandle, DVPP_CTL_VPC_PROC, &dvppApiCtlMsg);
    if (ret != 0) {
        HIAI_ENGINE_LOG(HIAI_ERROR, "call dvppctl fail");
        HIAI_DVPP_DFree(roiOutput->addr);
        DestroyDvppApi(dvppHandle);
        return;
    }
  //  ret = FrameReturnSaveVpcResult(frame, roiOutput->addr, imageCount);
  //  if (ret != 0) {
  //      HIAI_ENGINE_LOG(HIAI_ERROR, "save vpc result fail");
  //  }
    HIAI_DVPP_DFree(roiOutput->addr);
    DestroyDvppApi(dvppHandle);

    uint64_t now1 = Now(); 
    if (last_convert_ts[idx] == 0) {
        last_convert_ts[idx] = now1;
    } else {
        uint64_t delta = now1 - now;
        double delta_mill = delta / 1000000.0f;
        
//        printf("%d-frame: %d, convert time: %f\n", idx, imageCount,  delta_mill + temp_delta / 1000000.0f);
        g_delta_convert_time[idx] += delta;
        g_delta_convert_time[idx] += temp_delta;
        last_convert_ts[idx] = now1;

    }

    return;
}

void FrameReturn13(FRAME* frame, void* hiaiData)
{
    int idx = 13;
    uint64_t now = Now();

    static int32_t imageCount = 0;
    imageCount++;
   
    uint64_t temp_delta = 0; 
    if (last_codec_ts[idx] == 0) {         
	last_codec_ts[idx] = now;
    } else {
        temp_delta = now - last_codec_ts[idx];
//	printf("%d-frame: %d, codec time: %f\n", idx, imageCount,  (now - last_codec_ts[idx]) / 1000000.0f);
        g_delta_time[idx] += (now - last_codec_ts[idx]);
        g_frame_cnt[idx]++;
        last_codec_ts[idx] = now;
    }

    //HIAI_ENGINE_LOG("call save frame number:[%d], width:[%d], height:[%d]", imageCount, frame->width, frame->height);
//    printf("0-frame number:[%d], width:[%d], height:[%d]\n", imageCount, frame->width, frame->height);;
    // The image directly output by vdec is an image of hfbc compression format, which cannot be directly displayed.
    // It is necessary to call vpc to convert hfbc to an image of uncompressed format to display.
    //HIAI_ENGINE_LOG("start call vpc interface to translate hfbc.");
    IDVPPAPI* dvppHandle = nullptr;
    int32_t ret = CreateDvppApi(dvppHandle);
    if (ret != 0) {
        HIAI_ENGINE_LOG(HIAI_CREATE_DVPP_ERROR, "creat dvpp api fail.");
        return;
    }

    // Construct vpc input configuration.
    std::shared_ptr<VpcUserImageConfigure> userImage(new VpcUserImageConfigure);
    // bareDataAddr should be null which the image is hfbc.
    userImage->bareDataAddr = nullptr;
    userImage->bareDataBufferSize = 0;
    userImage->widthStride = frame->width;
    userImage->heightStride = frame->height;
    // Configuration input format
    string imageFormat(frame->image_format);
    if (frame->bitdepth == 8) {
        if (imageFormat == "nv12") {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_UV;
        } else {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_VU;
        }
    } else {
        if (imageFormat == "nv12") {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_UV_10BIT;
        } else {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_VU_10BIT;
        }
    }
    userImage->outputFormat = OUTPUT_YUV420SP_UV;
    userImage->isCompressData = true;
    // Configure hfbc input address
    VpcCompressDataConfigure* compressDataConfigure = &userImage->compressDataConfigure;
    uint64_t baseAddr = (uint64_t)frame->buffer;
    compressDataConfigure->lumaHeadAddr = baseAddr + frame->offset_head_y;
    compressDataConfigure->chromaHeadAddr = baseAddr + frame->offset_head_c;
    compressDataConfigure->lumaPayloadAddr = baseAddr + frame->offset_payload_y;
    compressDataConfigure->chromaPayloadAddr = baseAddr + frame->offset_payload_c;
    compressDataConfigure->lumaHeadStride = frame->stride_head;
    compressDataConfigure->chromaHeadStride = frame->stride_head;
    compressDataConfigure->lumaPayloadStride = frame->stride_payload;
    compressDataConfigure->chromaPayloadStride = frame->stride_payload;

    userImage->yuvSumEnable = false;
    userImage->cmdListBufferAddr = nullptr;
    userImage->cmdListBufferSize = 0;
    // Configure the roi area and output area
    std::shared_ptr<VpcUserRoiConfigure> roiConfigure(new VpcUserRoiConfigure);
    roiConfigure->next = nullptr;
    userImage->roiConfigure = roiConfigure.get();
    VpcUserRoiInputConfigure* roiInput = &roiConfigure->inputConfigure;
    roiInput->cropArea.leftOffset = 0;
    roiInput->cropArea.rightOffset = frame->width - 1;
    roiInput->cropArea.upOffset = 0;
    roiInput->cropArea.downOffset = frame->height - 1;
    VpcUserRoiOutputConfigure* roiOutput = &roiConfigure->outputConfigure;
    roiOutput->outputArea.leftOffset = 0;
    roiOutput->outputArea.rightOffset = frame->width - 1;
    roiOutput->outputArea.upOffset = 0;
    roiOutput->outputArea.downOffset = frame->height - 1;
    roiOutput->bufferSize = ALIGN_UP(frame->width, 16) * ALIGN_UP(frame->height, 2) * 3 / 2;
    roiOutput->addr = (uint8_t*)HIAI_DVPP_DMalloc(roiOutput->bufferSize);
    roiOutput->widthStride = ALIGN_UP(frame->width, 16);
    roiOutput->heightStride = ALIGN_UP(frame->height, 2);

    dvppapi_ctl_msg dvppApiCtlMsg;
    dvppApiCtlMsg.in = (void*)(userImage.get());
    dvppApiCtlMsg.in_size = sizeof(VpcUserImageConfigure);
    ret = DvppCtl(dvppHandle, DVPP_CTL_VPC_PROC, &dvppApiCtlMsg);
    if (ret != 0) {
        HIAI_ENGINE_LOG(HIAI_ERROR, "call dvppctl fail");
        HIAI_DVPP_DFree(roiOutput->addr);
        DestroyDvppApi(dvppHandle);
        return;
    }
  //  ret = FrameReturnSaveVpcResult(frame, roiOutput->addr, imageCount);
  //  if (ret != 0) {
  //      HIAI_ENGINE_LOG(HIAI_ERROR, "save vpc result fail");
  //  }
    HIAI_DVPP_DFree(roiOutput->addr);
    DestroyDvppApi(dvppHandle);

    uint64_t now1 = Now(); 
    if (last_convert_ts[idx] == 0) {
        last_convert_ts[idx] = now1;
    } else {
        uint64_t delta = now1 - now;
        double delta_mill = delta / 1000000.0f;
        
//        printf("%d-frame: %d, convert time: %f\n", idx, imageCount,  delta_mill + temp_delta / 1000000.0f);
        g_delta_convert_time[idx] += delta;
        g_delta_convert_time[idx] += temp_delta;
        last_convert_ts[idx] = now1;

    }

    return;
}

void FrameReturn14(FRAME* frame, void* hiaiData)
{
    int idx = 14;
    uint64_t now = Now();

    static int32_t imageCount = 0;
    imageCount++;
   
    uint64_t temp_delta = 0; 
    if (last_codec_ts[idx] == 0) {         
	last_codec_ts[idx] = now;
    } else {
        temp_delta = now - last_codec_ts[idx];
//	printf("%d-frame: %d, codec time: %f\n", idx, imageCount,  (now - last_codec_ts[idx]) / 1000000.0f);
        g_delta_time[idx] += (now - last_codec_ts[idx]);
        g_frame_cnt[idx]++;
        last_codec_ts[idx] = now;
    }

    //HIAI_ENGINE_LOG("call save frame number:[%d], width:[%d], height:[%d]", imageCount, frame->width, frame->height);
//    printf("0-frame number:[%d], width:[%d], height:[%d]\n", imageCount, frame->width, frame->height);;
    // The image directly output by vdec is an image of hfbc compression format, which cannot be directly displayed.
    // It is necessary to call vpc to convert hfbc to an image of uncompressed format to display.
    //HIAI_ENGINE_LOG("start call vpc interface to translate hfbc.");
    IDVPPAPI* dvppHandle = nullptr;
    int32_t ret = CreateDvppApi(dvppHandle);
    if (ret != 0) {
        HIAI_ENGINE_LOG(HIAI_CREATE_DVPP_ERROR, "creat dvpp api fail.");
        return;
    }

    // Construct vpc input configuration.
    std::shared_ptr<VpcUserImageConfigure> userImage(new VpcUserImageConfigure);
    // bareDataAddr should be null which the image is hfbc.
    userImage->bareDataAddr = nullptr;
    userImage->bareDataBufferSize = 0;
    userImage->widthStride = frame->width;
    userImage->heightStride = frame->height;
    // Configuration input format
    string imageFormat(frame->image_format);
    if (frame->bitdepth == 8) {
        if (imageFormat == "nv12") {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_UV;
        } else {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_VU;
        }
    } else {
        if (imageFormat == "nv12") {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_UV_10BIT;
        } else {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_VU_10BIT;
        }
    }
    userImage->outputFormat = OUTPUT_YUV420SP_UV;
    userImage->isCompressData = true;
    // Configure hfbc input address
    VpcCompressDataConfigure* compressDataConfigure = &userImage->compressDataConfigure;
    uint64_t baseAddr = (uint64_t)frame->buffer;
    compressDataConfigure->lumaHeadAddr = baseAddr + frame->offset_head_y;
    compressDataConfigure->chromaHeadAddr = baseAddr + frame->offset_head_c;
    compressDataConfigure->lumaPayloadAddr = baseAddr + frame->offset_payload_y;
    compressDataConfigure->chromaPayloadAddr = baseAddr + frame->offset_payload_c;
    compressDataConfigure->lumaHeadStride = frame->stride_head;
    compressDataConfigure->chromaHeadStride = frame->stride_head;
    compressDataConfigure->lumaPayloadStride = frame->stride_payload;
    compressDataConfigure->chromaPayloadStride = frame->stride_payload;

    userImage->yuvSumEnable = false;
    userImage->cmdListBufferAddr = nullptr;
    userImage->cmdListBufferSize = 0;
    // Configure the roi area and output area
    std::shared_ptr<VpcUserRoiConfigure> roiConfigure(new VpcUserRoiConfigure);
    roiConfigure->next = nullptr;
    userImage->roiConfigure = roiConfigure.get();
    VpcUserRoiInputConfigure* roiInput = &roiConfigure->inputConfigure;
    roiInput->cropArea.leftOffset = 0;
    roiInput->cropArea.rightOffset = frame->width - 1;
    roiInput->cropArea.upOffset = 0;
    roiInput->cropArea.downOffset = frame->height - 1;
    VpcUserRoiOutputConfigure* roiOutput = &roiConfigure->outputConfigure;
    roiOutput->outputArea.leftOffset = 0;
    roiOutput->outputArea.rightOffset = frame->width - 1;
    roiOutput->outputArea.upOffset = 0;
    roiOutput->outputArea.downOffset = frame->height - 1;
    roiOutput->bufferSize = ALIGN_UP(frame->width, 16) * ALIGN_UP(frame->height, 2) * 3 / 2;
    roiOutput->addr = (uint8_t*)HIAI_DVPP_DMalloc(roiOutput->bufferSize);
    roiOutput->widthStride = ALIGN_UP(frame->width, 16);
    roiOutput->heightStride = ALIGN_UP(frame->height, 2);

    dvppapi_ctl_msg dvppApiCtlMsg;
    dvppApiCtlMsg.in = (void*)(userImage.get());
    dvppApiCtlMsg.in_size = sizeof(VpcUserImageConfigure);
    ret = DvppCtl(dvppHandle, DVPP_CTL_VPC_PROC, &dvppApiCtlMsg);
    if (ret != 0) {
        HIAI_ENGINE_LOG(HIAI_ERROR, "call dvppctl fail");
        HIAI_DVPP_DFree(roiOutput->addr);
        DestroyDvppApi(dvppHandle);
        return;
    }
  //  ret = FrameReturnSaveVpcResult(frame, roiOutput->addr, imageCount);
  //  if (ret != 0) {
  //      HIAI_ENGINE_LOG(HIAI_ERROR, "save vpc result fail");
  //  }
    HIAI_DVPP_DFree(roiOutput->addr);
    DestroyDvppApi(dvppHandle);

    uint64_t now1 = Now(); 
    if (last_convert_ts[idx] == 0) {
        last_convert_ts[idx] = now1;
    } else {
        uint64_t delta = now1 - now;
        double delta_mill = delta / 1000000.0f;
        
//        printf("%d-frame: %d, convert time: %f\n", idx, imageCount,  delta_mill + temp_delta / 1000000.0f);
        g_delta_convert_time[idx] += delta;
        g_delta_convert_time[idx] += temp_delta;
        last_convert_ts[idx] = now1;

    }

    return;
}

void FrameReturn15(FRAME* frame, void* hiaiData)
{
    int idx = 15;
    uint64_t now = Now();

    static int32_t imageCount = 0;
    imageCount++;
   
    uint64_t temp_delta = 0; 
    if (last_codec_ts[idx] == 0) {         
	last_codec_ts[idx] = now;
    } else {
        temp_delta = now - last_codec_ts[idx];
//	printf("%d-frame: %d, codec time: %f\n", idx, imageCount,  (now - last_codec_ts[idx]) / 1000000.0f);
        g_delta_time[idx] += (now - last_codec_ts[idx]);
        g_frame_cnt[idx]++;
        last_codec_ts[idx] = now;
    }

    //HIAI_ENGINE_LOG("call save frame number:[%d], width:[%d], height:[%d]", imageCount, frame->width, frame->height);
//    printf("0-frame number:[%d], width:[%d], height:[%d]\n", imageCount, frame->width, frame->height);;
    // The image directly output by vdec is an image of hfbc compression format, which cannot be directly displayed.
    // It is necessary to call vpc to convert hfbc to an image of uncompressed format to display.
    //HIAI_ENGINE_LOG("start call vpc interface to translate hfbc.");
    IDVPPAPI* dvppHandle = nullptr;
    int32_t ret = CreateDvppApi(dvppHandle);
    if (ret != 0) {
        HIAI_ENGINE_LOG(HIAI_CREATE_DVPP_ERROR, "creat dvpp api fail.");
        return;
    }

    // Construct vpc input configuration.
    std::shared_ptr<VpcUserImageConfigure> userImage(new VpcUserImageConfigure);
    // bareDataAddr should be null which the image is hfbc.
    userImage->bareDataAddr = nullptr;
    userImage->bareDataBufferSize = 0;
    userImage->widthStride = frame->width;
    userImage->heightStride = frame->height;
    // Configuration input format
    string imageFormat(frame->image_format);
    if (frame->bitdepth == 8) {
        if (imageFormat == "nv12") {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_UV;
        } else {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_VU;
        }
    } else {
        if (imageFormat == "nv12") {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_UV_10BIT;
        } else {
            userImage->inputFormat = INPUT_YUV420_SEMI_PLANNER_VU_10BIT;
        }
    }
    userImage->outputFormat = OUTPUT_YUV420SP_UV;
    userImage->isCompressData = true;
    // Configure hfbc input address
    VpcCompressDataConfigure* compressDataConfigure = &userImage->compressDataConfigure;
    uint64_t baseAddr = (uint64_t)frame->buffer;
    compressDataConfigure->lumaHeadAddr = baseAddr + frame->offset_head_y;
    compressDataConfigure->chromaHeadAddr = baseAddr + frame->offset_head_c;
    compressDataConfigure->lumaPayloadAddr = baseAddr + frame->offset_payload_y;
    compressDataConfigure->chromaPayloadAddr = baseAddr + frame->offset_payload_c;
    compressDataConfigure->lumaHeadStride = frame->stride_head;
    compressDataConfigure->chromaHeadStride = frame->stride_head;
    compressDataConfigure->lumaPayloadStride = frame->stride_payload;
    compressDataConfigure->chromaPayloadStride = frame->stride_payload;

    userImage->yuvSumEnable = false;
    userImage->cmdListBufferAddr = nullptr;
    userImage->cmdListBufferSize = 0;
    // Configure the roi area and output area
    std::shared_ptr<VpcUserRoiConfigure> roiConfigure(new VpcUserRoiConfigure);
    roiConfigure->next = nullptr;
    userImage->roiConfigure = roiConfigure.get();
    VpcUserRoiInputConfigure* roiInput = &roiConfigure->inputConfigure;
    roiInput->cropArea.leftOffset = 0;
    roiInput->cropArea.rightOffset = frame->width - 1;
    roiInput->cropArea.upOffset = 0;
    roiInput->cropArea.downOffset = frame->height - 1;
    VpcUserRoiOutputConfigure* roiOutput = &roiConfigure->outputConfigure;
    roiOutput->outputArea.leftOffset = 0;
    roiOutput->outputArea.rightOffset = frame->width - 1;
    roiOutput->outputArea.upOffset = 0;
    roiOutput->outputArea.downOffset = frame->height - 1;
    roiOutput->bufferSize = ALIGN_UP(frame->width, 16) * ALIGN_UP(frame->height, 2) * 3 / 2;
    roiOutput->addr = (uint8_t*)HIAI_DVPP_DMalloc(roiOutput->bufferSize);
    roiOutput->widthStride = ALIGN_UP(frame->width, 16);
    roiOutput->heightStride = ALIGN_UP(frame->height, 2);

    dvppapi_ctl_msg dvppApiCtlMsg;
    dvppApiCtlMsg.in = (void*)(userImage.get());
    dvppApiCtlMsg.in_size = sizeof(VpcUserImageConfigure);
    ret = DvppCtl(dvppHandle, DVPP_CTL_VPC_PROC, &dvppApiCtlMsg);
    if (ret != 0) {
        HIAI_ENGINE_LOG(HIAI_ERROR, "call dvppctl fail");
        HIAI_DVPP_DFree(roiOutput->addr);
        DestroyDvppApi(dvppHandle);
        return;
    }
  //  ret = FrameReturnSaveVpcResult(frame, roiOutput->addr, imageCount);
  //  if (ret != 0) {
  //      HIAI_ENGINE_LOG(HIAI_ERROR, "save vpc result fail");
  //  }
    HIAI_DVPP_DFree(roiOutput->addr);
    DestroyDvppApi(dvppHandle);

    uint64_t now1 = Now(); 
    if (last_convert_ts[idx] == 0) {
        last_convert_ts[idx] = now1;
    } else {
        uint64_t delta = now1 - now;
        double delta_mill = delta / 1000000.0f;
        
//        printf("%d-frame: %d, convert time: %f\n", idx, imageCount,  delta_mill + temp_delta / 1000000.0f);
        g_delta_convert_time[idx] += delta;
        g_delta_convert_time[idx] += temp_delta;
        last_convert_ts[idx] = now1;

    }

    return;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

int32_t FrameReturnSaveVpcResult(FRAME* frame, uint8_t* addr, int32_t imageCount)
{
    char vpcfileName[50] = {0};
    int32_t safeFuncRet = strncpy_s(vpcfileName, sizeof(vpcfileName), "output_dir/image_vpc",
        strlen("output_dir/image_vpc"));
    if (safeFuncRet != EOK) {
        HIAI_ENGINE_LOG(HIAI_ERROR, "strncpy_s fail, ret = %d", safeFuncRet);
        return -1;
    }
    safeFuncRet = sprintf_s(vpcfileName + strlen("output_dir/image_vpc"),
        sizeof(vpcfileName) - strlen("output_dir/image_vpc"), "%d", imageCount);
    if (safeFuncRet == -1) {
        HIAI_ENGINE_LOG(HIAI_ERROR, "sprintf_s fail, ret = %d", safeFuncRet);
        return -1;
    }
    FILE* fp_vpc = nullptr;
    fp_vpc = fopen(vpcfileName, "wb+");
    if (fp_vpc == nullptr) {
        HIAI_ENGINE_LOG(HIAI_OPEN_FILE_ERROR, "open file %s failed~", vpcfileName);
        return -1;
    }

    char* out_buffer = (char*)addr;
    int out_image_width = frame->realWidth;
    int out_image_high = frame->realHeight;
    int out_image_len = out_image_width * out_image_high * 3 / 2;
    char* out_image_buffer = (char*)malloc(out_image_len);
    int out_align_width = frame->width;
    int out_align_high = frame->height;
    // copy valid yuv data
    for (int i = 0; i < out_image_high; i++) {
        safeFuncRet = memcpy_s(out_image_buffer + i * out_image_width, out_image_len,
            out_buffer + i * out_align_width, out_image_width);
        if (safeFuncRet != EOK) {
            HIAI_ENGINE_LOG(HIAI_ERROR, "memcpy_s fail");
            break;
        }
    }
    for (int i = 0; i < out_image_high / 2; i++) {
        safeFuncRet = memcpy_s(out_image_buffer + i * out_image_width + out_image_width * out_image_high, out_image_len,
            out_buffer + i * out_align_width + out_align_width * out_align_high, out_image_width);
        if (safeFuncRet != EOK) {
            HIAI_ENGINE_LOG(HIAI_ERROR, "memcpy_s fail");
            break;
        }
    }

    fwrite(out_image_buffer, 1, out_image_len, fp_vpc);
    HIAI_ENGINE_LOG("save vpc result:%s, write size: %d", vpcfileName, out_image_len);
    fflush(fp_vpc);
    fclose(fp_vpc);
    free(out_image_buffer);
    out_image_buffer = nullptr;
    return 0;
}

/*
 * get the buffer size.
 */
void GetInBufferSize(uint32_t& inBufferSize)
{
    switch (g_format) {
        case INPUT_YUV400:
        case INPUT_YUV420_SEMI_PLANNER_UV:
        case INPUT_YUV420_SEMI_PLANNER_VU:
            inBufferSize = g_inWidthStride * g_inHighStride * 3 / 2;
            break;
        case INPUT_YUV422_SEMI_PLANNER_UV:
        case INPUT_YUV422_SEMI_PLANNER_VU:
            inBufferSize = g_inWidthStride * g_inHighStride * 2;
            break;
        case INPUT_YUV444_SEMI_PLANNER_UV:
        case INPUT_YUV444_SEMI_PLANNER_VU:
            inBufferSize = g_inWidthStride * g_inHighStride * 3;
            break;
        case INPUT_YUV422_PACKED_YUYV:
        case INPUT_YUV422_PACKED_UYVY:
        case INPUT_YUV422_PACKED_YVYU:
        case INPUT_YUV422_PACKED_VYUY:
        case INPUT_YUV444_PACKED_YUV:
        case INPUT_RGB:
        case INPUT_BGR:
        case INPUT_ARGB:
        case INPUT_ABGR:
        case INPUT_RGBA:
        case INPUT_BGRA:
            inBufferSize = g_inWidthStride * g_inHighStride;
            break;
        default:
            inBufferSize = 0;
            break;
    }
}

/*
* VPC new interface，implement vpc basic functions
*/
void TEST_VPC()
{
    uint32_t inWidthStride = g_inWidthStride;
    uint32_t inHighStride = g_inHighStride;
    uint32_t outWidthStride = g_outWidthStride;
    uint32_t outHighStride = g_outHighStride;
    uint32_t inBufferSize = 0;
    GetInBufferSize(inBufferSize);

    uint32_t outBufferSize = outWidthStride * outHighStride * 3 / 2;
    uint8_t* inBuffer  = static_cast<uint8_t*>(HIAI_DVPP_DMalloc(inBufferSize)); // 申请输入buffer
    uint8_t* outBuffer = static_cast<uint8_t*>(HIAI_DVPP_DMalloc(outBufferSize)); // 申请输出buffer

    uint32_t inFormat = g_format;
    uint32_t outFormat = g_outFormat;

    FILE* fp = fopen(in_file_name, "rb+");
    if (fp == nullptr) {
        HIAI_ENGINE_LOG(HIAI_OPEN_FILE_ERROR, "fopen file failed");
        fclose(fp);
        return;
    }

    fread(inBuffer, 1, inBufferSize, fp);
    fclose(fp);
    // Construct an input image configuration.
    std::shared_ptr<VpcUserImageConfigure> imageConfigure(new VpcUserImageConfigure);
    imageConfigure->bareDataAddr = reinterpret_cast<uint8_t*>(inBuffer);
    imageConfigure->bareDataBufferSize = inBufferSize;
    imageConfigure->widthStride = inWidthStride;
    imageConfigure->heightStride = inHighStride;
    HIAI_ENGINE_LOG("inWidthStride = %u", inWidthStride);
    HIAI_ENGINE_LOG("inHighStride = %u", inHighStride);

    imageConfigure->inputFormat = (VpcInputFormat)inFormat;
    imageConfigure->outputFormat = (VpcOutputFormat)outFormat;
    HIAI_ENGINE_LOG("inFormat= %u", imageConfigure->inputFormat);
    HIAI_ENGINE_LOG("outFormat= %u", imageConfigure->outputFormat);
    imageConfigure->yuvSumEnable = false;
    imageConfigure->cmdListBufferAddr = nullptr;
    imageConfigure->cmdListBufferSize = 0;
    std::shared_ptr<VpcUserRoiConfigure> roiConfigure(new VpcUserRoiConfigure);
    roiConfigure->next = nullptr;
    VpcUserRoiInputConfigure* inputConfigure = &roiConfigure->inputConfigure;
    // Set the roi area, the top left corner coordinate of the roi area [0,0].
    inputConfigure->cropArea.leftOffset  = g_hMin;
    inputConfigure->cropArea.rightOffset = g_hMax;
    inputConfigure->cropArea.upOffset    = g_vMin;
    inputConfigure->cropArea.downOffset  = g_vMax;
    HIAI_ENGINE_LOG("cropArea.leftOffset = %u", inputConfigure->cropArea.leftOffset);
    HIAI_ENGINE_LOG("cropArea.rightOffset = %u", inputConfigure->cropArea.rightOffset);
    HIAI_ENGINE_LOG("cropArea.upOffset = %u", inputConfigure->cropArea.upOffset);
    HIAI_ENGINE_LOG("cropArea.downOffset = %u", inputConfigure->cropArea.downOffset);

    VpcUserRoiOutputConfigure* outputConfigure = &roiConfigure->outputConfigure;
    outputConfigure->addr = outBuffer;
    outputConfigure->bufferSize = outBufferSize;
    outputConfigure->widthStride = outWidthStride;
    outputConfigure->heightStride = outHighStride;
    HIAI_ENGINE_LOG("outputConfigure->widthStride = %u", outputConfigure->widthStride);
    HIAI_ENGINE_LOG("outputConfigure->heightStride = %u",  outputConfigure->heightStride);
    // Set the texture area, the top left corner of the texture area [0,0].
    outputConfigure->outputArea.leftOffset  = g_outLeftOffset;
    outputConfigure->outputArea.rightOffset = g_outRightOffset;
    outputConfigure->outputArea.upOffset    = g_outUpOffset;
    outputConfigure->outputArea.downOffset  = g_outDownOffset;
    HIAI_ENGINE_LOG("outputArea.leftOffset = %u", outputConfigure->outputArea.leftOffset);
    HIAI_ENGINE_LOG("outputArea.rightOffset = %u", outputConfigure->outputArea.rightOffset);
    HIAI_ENGINE_LOG("outputArea.upOffset = %u", outputConfigure->outputArea.upOffset);
    HIAI_ENGINE_LOG("outputArea.downOffset = %u", outputConfigure->outputArea.downOffset);

    imageConfigure->roiConfigure = roiConfigure.get();

    IDVPPAPI *pidvppapi = nullptr;
    int32_t ret = CreateDvppApi(pidvppapi);
    if (ret != 0) {
        HIAI_ENGINE_LOG(HIAI_CREATE_DVPP_ERROR, "create dvpp api fail.");
        HIAI_DVPP_DFree(inBuffer);
        HIAI_DVPP_DFree(outBuffer);
        inBuffer = nullptr;
        outBuffer = nullptr;
        return;
    }
    dvppapi_ctl_msg dvppApiCtlMsg;
    dvppApiCtlMsg.in = static_cast<void*>(imageConfigure.get());
    dvppApiCtlMsg.in_size = sizeof(VpcUserImageConfigure);

    for (int32_t i = 0; i < g_loop; i++) {
        ret = DvppCtl(pidvppapi, DVPP_CTL_VPC_PROC, &dvppApiCtlMsg);
        if (ret != 0) {
            HIAI_ENGINE_LOG(HIAI_VPC_CTL_ERROR, "call vpc dvppctl process faild!");
            ret = DestroyDvppApi(pidvppapi);
            HIAI_DVPP_DFree(inBuffer);
            HIAI_DVPP_DFree(outBuffer);
            inBuffer = nullptr;
            outBuffer = nullptr;
            return;
        } else {
            HIAI_ENGINE_LOG(HIAI_OK, "call vpc dvppctl process success!");
        }
    }

    FILE* outImageFp = nullptr;
    if (g_transform == 0) {
        uint32_t outImageWidth = (g_outRightOffset - g_outLeftOffset + 1);
        uint32_t outImageHigh = (g_outDownOffset - g_outUpOffset + 1);

        int outImageLen = outImageWidth * outImageHigh * 3 / 2;
        int outAlignWidth = ALIGN_UP(outImageWidth, 16);
        int outAlignHigh = ALIGN_UP(outImageHigh, 2);
        HIAI_ENGINE_LOG("outImageWidth = %u, outImageHigh = %u, outAlignWidth = %u, outAlignHigh = %u",
            outImageWidth, outImageHigh, outAlignWidth, outAlignHigh);
        char* outImageBuffer = (char*)malloc(outImageLen);
        int32_t safeFuncRet = 0;
        for (int i = 0; i < outImageHigh; i++) {
            safeFuncRet = memcpy_s(outImageBuffer + i * outImageWidth, outImageLen,
                outBuffer + i * outAlignWidth, outImageWidth);
            if (safeFuncRet != EOK) {
                HIAI_ENGINE_LOG(HIAI_ERROR, "memcpy_s fail");
                DestroyDvppApi(pidvppapi);
                HIAI_DVPP_DFree(inBuffer);
                HIAI_DVPP_DFree(outBuffer);
                free(outImageBuffer);
                return;
            }
        }
        for (int i = 0; i < outImageHigh / 2; i++) {
            safeFuncRet = memcpy_s(outImageBuffer + i * outImageWidth + outImageWidth * outImageHigh, outImageLen,
            outBuffer + i * outAlignWidth + outAlignWidth * outAlignHigh, outImageWidth);
            if (safeFuncRet != EOK) {
                HIAI_ENGINE_LOG(HIAI_ERROR, "memcpy_s fail");
                DestroyDvppApi(pidvppapi);
                HIAI_DVPP_DFree(inBuffer);
                HIAI_DVPP_DFree(outBuffer);
                free(outImageBuffer);
                return;
            }
        }

        outImageFp = fopen(out_file_name, "wb+");
        if (outImageFp == nullptr) {
            HIAI_ENGINE_LOG(HIAI_OPEN_FILE_ERROR, "fopen out file failed");
            fclose(outImageFp);
            DestroyDvppApi(pidvppapi);
            HIAI_DVPP_DFree(inBuffer);
            HIAI_DVPP_DFree(outBuffer);
            free(outImageBuffer);
            return;
        }
        fwrite(outImageBuffer, 1, outImageLen, outImageFp);
        free(outImageBuffer);
    } else {
        outImageFp = fopen(out_file_name, "wb+");
        fwrite(outBuffer, 1, outBufferSize, outImageFp);
    }
    fclose(outImageFp);
    ret = DestroyDvppApi(pidvppapi);
    HIAI_DVPP_DFree(inBuffer);
    HIAI_DVPP_DFree(outBuffer);
    inBuffer = nullptr;
    outBuffer = nullptr;

    return;
}

/*
 * test for Original zoom.
 */
void NewVpcTest1()
{
    uint32_t inWidthStride = 1920;
    uint32_t inHeightStride = 1080;
    uint32_t outWidthStride = 1280;
    uint32_t outHeightStride = 720;
    uint32_t inBufferSize = inWidthStride * inHeightStride * 3 / 2; // 1080P yuv420sp Image
    uint32_t outBufferSize = outWidthStride * outHeightStride * 3 / 2; // 720P yuv420sp bImage
    uint8_t* inBuffer  = static_cast<uint8_t*>(HIAI_DVPP_DMalloc(inBufferSize)); // Construct an input picture.
    uint8_t* outBuffer = static_cast<uint8_t*>(HIAI_DVPP_DMalloc(outBufferSize)); // Construct an output picture.

    FILE* fp = fopen("dvpp_vpc_1920x1080_nv12.yuv", "rb+");
    if (fp == nullptr) {
        HIAI_ENGINE_LOG(HIAI_ERROR, "fopen file failed");
        fclose(fp);
        return;
    }

    fread(inBuffer, 1, inBufferSize, fp);
    fclose(fp);
    // Construct the input picture configuration.
    std::shared_ptr<VpcUserImageConfigure> imageConfigure(new VpcUserImageConfigure);
    imageConfigure->bareDataAddr = inBuffer;
    imageConfigure->bareDataBufferSize = inBufferSize;
    imageConfigure->widthStride = inWidthStride;
    imageConfigure->heightStride = inHeightStride;
    imageConfigure->inputFormat = INPUT_YUV420_SEMI_PLANNER_UV;
    imageConfigure->outputFormat = OUTPUT_YUV420SP_UV;
    imageConfigure->yuvSumEnable = false;
    imageConfigure->cmdListBufferAddr = nullptr;
    imageConfigure->cmdListBufferSize = 0;
    std::shared_ptr<VpcUserRoiConfigure> roiConfigure(new VpcUserRoiConfigure);
    roiConfigure->next = nullptr;
    VpcUserRoiInputConfigure* inputConfigure = &roiConfigure->inputConfigure;
    // Set the drawing area, [0,0] in the upper left corner of the area,
    // and [1919,1079] in the lower right corner.
    inputConfigure->cropArea.leftOffset  = 0;
    inputConfigure->cropArea.rightOffset = inWidthStride - 1;
    inputConfigure->cropArea.upOffset    = 0;
    inputConfigure->cropArea.downOffset  = inHeightStride - 1;
    VpcUserRoiOutputConfigure* outputConfigure = &roiConfigure->outputConfigure;
    outputConfigure->addr = outBuffer;
    outputConfigure->bufferSize = outBufferSize;
    outputConfigure->widthStride = outWidthStride;
    outputConfigure->heightStride = outHeightStride;
    // Set the map area, coordinate [0,0] in the upper left corner of the map area,
    // and [1279,719] in the lower right corner.
    outputConfigure->outputArea.leftOffset  = 0;
    outputConfigure->outputArea.rightOffset = outWidthStride - 1;
    outputConfigure->outputArea.upOffset    = 0;
    outputConfigure->outputArea.downOffset  = outHeightStride - 1;

    imageConfigure->roiConfigure = roiConfigure.get();

    IDVPPAPI *pidvppapi = nullptr;
    int32_t ret = CreateDvppApi(pidvppapi);
    if (ret != 0) {
        HIAI_ENGINE_LOG(HIAI_CREATE_DVPP_ERROR, "create dvpp api fail.");
        HIAI_DVPP_DFree(inBuffer);
        HIAI_DVPP_DFree(outBuffer);
        return;
    }
    dvppapi_ctl_msg dvppApiCtlMsg;
    dvppApiCtlMsg.in = static_cast<void*>(imageConfigure.get());
    dvppApiCtlMsg.in_size = sizeof(VpcUserImageConfigure);
    ret = DvppCtl(pidvppapi, DVPP_CTL_VPC_PROC, &dvppApiCtlMsg);
    if (ret != 0) {
        HIAI_ENGINE_LOG(HIAI_VPC_CTL_ERROR, "call vpc dvppctl process faild!");
        ret = DestroyDvppApi(pidvppapi);
        HIAI_DVPP_DFree(inBuffer);
        HIAI_DVPP_DFree(outBuffer);
        return;
    } else {
        HIAI_ENGINE_LOG(HIAI_OK, "NewVpcTest1::call vpc dvppctl process success!");
    }

    FILE* outImageFp = fopen("NewVpcTest1Out.yuv", "wb+");
    fwrite(outBuffer, 1, outBufferSize, outImageFp);

    ret = DestroyDvppApi(pidvppapi);

    HIAI_DVPP_DFree(inBuffer);
    HIAI_DVPP_DFree(outBuffer);
    fclose(outImageFp);
    return;
}

/*
 * roi zoom , the result is output to the specified position of the output image.
 */
void NewVpcTest2()
{
    uint32_t inWidthStride = 1920;
    uint32_t inHeightStride = 1080;
    uint32_t outWidthStride = 1280;
    uint32_t outHeightStride = 720;
    uint32_t inBufferSize = inWidthStride * inHeightStride * 3 / 2; // 1080P yuv420sp Image
    uint32_t outBufferSize = outWidthStride * outHeightStride * 3 / 2; // 720P yuv420sp Image
    uint8_t* inBuffer  = static_cast<uint8_t*>(HIAI_DVPP_DMalloc(inBufferSize)); //  Construct an input picture.
    uint8_t* outBuffer = static_cast<uint8_t*>(HIAI_DVPP_DMalloc(outBufferSize)); //  Construct an output picture.

    FILE* fp = fopen("dvpp_vpc_1920x1080_nv12.yuv", "rb+");
    if (fp == nullptr) {
        HIAI_ENGINE_LOG(HIAI_ERROR, "fopen file failed");
        fclose(fp);
        return;
    }

    fread(inBuffer, 1, inBufferSize, fp);
    fclose(fp);
    // Construct the input picture configuration
    std::shared_ptr<VpcUserImageConfigure> imageConfigure(new VpcUserImageConfigure);
    imageConfigure->bareDataAddr = inBuffer;
    imageConfigure->bareDataBufferSize = inBufferSize;
    imageConfigure->widthStride = inWidthStride;
    imageConfigure->heightStride = inHeightStride;
    imageConfigure->inputFormat = INPUT_YUV420_SEMI_PLANNER_UV;
    imageConfigure->outputFormat = OUTPUT_YUV420SP_UV;
    imageConfigure->yuvSumEnable = false;
    imageConfigure->cmdListBufferAddr = nullptr;
    imageConfigure->cmdListBufferSize = 0;
    std::shared_ptr<VpcUserRoiConfigure> roiConfigure(new VpcUserRoiConfigure);
    roiConfigure->next = nullptr;
    VpcUserRoiInputConfigure* inputConfigure = &roiConfigure->inputConfigure;
    // Set the drawing area, [100,100] in the upper left corner of the area, and [499,499] in the lower right corner.
    inputConfigure->cropArea.leftOffset  = 100;
    inputConfigure->cropArea.rightOffset = 499;
    inputConfigure->cropArea.upOffset    = 100;
    inputConfigure->cropArea.downOffset  = 499;
    VpcUserRoiOutputConfigure* outputConfigure = &roiConfigure->outputConfigure;
    outputConfigure->addr = outBuffer;
    outputConfigure->bufferSize = outBufferSize;
    outputConfigure->widthStride = outWidthStride;
    outputConfigure->heightStride = outHeightStride;
    // Set the map area, [256,200] in the upper left corner of the map area, and [399,399] in the lower right corner.
    outputConfigure->outputArea.leftOffset  = 256; // The offset value must be 16-pixel-aligned.
    outputConfigure->outputArea.rightOffset = 399;
    outputConfigure->outputArea.upOffset    = 200;
    outputConfigure->outputArea.downOffset  = 399;

    imageConfigure->roiConfigure = roiConfigure.get();

    IDVPPAPI *pidvppapi = nullptr;
    int32_t ret = CreateDvppApi(pidvppapi);
    if (ret != 0) {
        HIAI_ENGINE_LOG(HIAI_CREATE_DVPP_ERROR, "create dvpp api fail.");
        HIAI_DVPP_DFree(inBuffer);
        HIAI_DVPP_DFree(outBuffer);
        return;
    }
    dvppapi_ctl_msg dvppApiCtlMsg;
    dvppApiCtlMsg.in = static_cast<void*>(imageConfigure.get());
    dvppApiCtlMsg.in_size = sizeof(VpcUserImageConfigure);
    ret = DvppCtl(pidvppapi, DVPP_CTL_VPC_PROC, &dvppApiCtlMsg);
    if (ret != 0) {
        HIAI_ENGINE_LOG(HIAI_VPC_CTL_ERROR, "call vpc dvppctl process faild!");
        ret = DestroyDvppApi(pidvppapi);
        HIAI_DVPP_DFree(inBuffer);
        HIAI_DVPP_DFree(outBuffer);
        return;
    } else {
        HIAI_ENGINE_LOG(HIAI_OK, "NewVpcTest2::call vpc dvppctl process success!");
    }

    FILE* outImageFp = fopen("NewVpcTest2Out.yuv", "wb+");
    fwrite(outBuffer, 1, outBufferSize, outImageFp);

    ret = DestroyDvppApi(pidvppapi);
    HIAI_DVPP_DFree(inBuffer);
    HIAI_DVPP_DFree(outBuffer);
    fclose(outImageFp);
    return;
}

/*
 * roi multiple images.
 */
void NewVpcTest3()
{
    uint32_t inWidthStride = 1920;
    uint32_t inHeightStride = 1080;
    uint32_t outWidthStride = 1280;
    uint32_t outHeightStride = 720;
    uint32_t inBufferSize = inWidthStride * inHeightStride * 3 / 2; // 1080P yuv420sp Image
    uint32_t outBufferSize = outWidthStride * outHeightStride * 3 / 2; // 720P yuv420sp Image
    uint8_t* inBuffer = static_cast<uint8_t*>(HIAI_DVPP_DMalloc(inBufferSize)); // Construct an input picture.

    FILE* fp = fopen("dvpp_vpc_1920x1080_nv12.yuv", "rb+");
    if (fp == nullptr) {
        HIAI_ENGINE_LOG(HIAI_ERROR, "fopen file failed");
        fclose(fp);
        return;
    }

    fread(inBuffer, 1, inBufferSize, fp);
    fclose(fp);
    fp == nullptr;
    // Construct the input picture configuration.
    std::shared_ptr<VpcUserImageConfigure> imageConfigure(new VpcUserImageConfigure);
    imageConfigure->bareDataAddr = inBuffer;
    imageConfigure->bareDataBufferSize = inBufferSize;
    imageConfigure->widthStride = inWidthStride;
    imageConfigure->heightStride = inHeightStride;
    imageConfigure->inputFormat = INPUT_YUV420_SEMI_PLANNER_UV;
    imageConfigure->outputFormat = OUTPUT_YUV420SP_UV;
    imageConfigure->yuvSumEnable = false;
    imageConfigure->cmdListBufferAddr = nullptr;
    imageConfigure->cmdListBufferSize = 0;
    std::shared_ptr<VpcUserRoiConfigure> lastRoi;
    std::vector<std::shared_ptr<VpcUserRoiConfigure>> roiVector;
    for (uint32_t i = 0; i < 5; i++) {
        std::shared_ptr<VpcUserRoiConfigure> roiConfigure(new VpcUserRoiConfigure);
        roiVector.push_back(roiConfigure);
        roiConfigure->next = nullptr;
        VpcUserRoiInputConfigure* inputConfigure = &roiConfigure->inputConfigure;
        // Set the drawing area.
        inputConfigure->cropArea.leftOffset  = 100 + i * 16;
        inputConfigure->cropArea.rightOffset = 499 + i * 16;
        inputConfigure->cropArea.upOffset    = 100 + i * 16;
        inputConfigure->cropArea.downOffset  = 499 + i * 16;
        VpcUserRoiOutputConfigure* outputConfigure = &roiConfigure->outputConfigure;
        uint8_t* outBuffer = static_cast<uint8_t*>(HIAI_DVPP_DMalloc(outBufferSize)); // Construct an input picture
        outputConfigure->addr = outBuffer;
        outputConfigure->bufferSize = outBufferSize;
        outputConfigure->widthStride = outWidthStride;
        outputConfigure->heightStride = outHeightStride;
        // Set the map area.
        outputConfigure->outputArea.leftOffset  = 256 + i * 16; // The offset value must be 16-pixel-aligned.
        outputConfigure->outputArea.rightOffset = 399 + i * 16;
        outputConfigure->outputArea.upOffset    = 256 + i * 16;
        outputConfigure->outputArea.downOffset  = 399 + i * 16;
        if (i == 0) {
            imageConfigure->roiConfigure = roiConfigure.get();
            lastRoi = roiConfigure;
        } else {
            lastRoi->next = roiConfigure.get();
            lastRoi = roiConfigure;
        }
    }

    IDVPPAPI *pidvppapi = nullptr;
    int32_t ret = CreateDvppApi(pidvppapi);
    if (ret != 0) {
        HIAI_ENGINE_LOG(HIAI_CREATE_DVPP_ERROR, "create dvpp api fail.");
        HIAI_DVPP_DFree(inBuffer);
        while (imageConfigure->roiConfigure != nullptr) {
            HIAI_DVPP_DFree(imageConfigure->roiConfigure->outputConfigure.addr);
            imageConfigure->roiConfigure = imageConfigure->roiConfigure->next;
        }
        return;
    }
    dvppapi_ctl_msg dvppApiCtlMsg;
    dvppApiCtlMsg.in = static_cast<void*>(imageConfigure.get());
    dvppApiCtlMsg.in_size = sizeof(VpcUserImageConfigure);
    ret = DvppCtl(pidvppapi, DVPP_CTL_VPC_PROC, &dvppApiCtlMsg);
    if (ret != 0) {
        HIAI_ENGINE_LOG(HIAI_VPC_CTL_ERROR, "call vpc dvppctl process faild!");
        ret = DestroyDvppApi(pidvppapi);
        HIAI_DVPP_DFree(inBuffer);
        while (imageConfigure->roiConfigure != nullptr) {
            HIAI_DVPP_DFree(imageConfigure->roiConfigure->outputConfigure.addr);
            imageConfigure->roiConfigure = imageConfigure->roiConfigure->next;
        }
        return;
    } else {
        HIAI_ENGINE_LOG(HIAI_OK, "NewVpcTest3::call vpc dvppctl process success!");
    }
    FILE* outImageFp = nullptr;
    uint32_t imageCount = 0;
    char fileName[50] = {0};

    while (imageConfigure->roiConfigure != nullptr) {
        int32_t safeFuncRet = sprintf_s(fileName, sizeof(fileName), "NewVpcTest3_%dOut.yuv", imageCount);
        if (safeFuncRet == -1) {
            DestroyDvppApi(pidvppapi);
            HIAI_DVPP_DFree(inBuffer);
            while (imageConfigure->roiConfigure != nullptr) {
                HIAI_DVPP_DFree(imageConfigure->roiConfigure->outputConfigure.addr);
                imageConfigure->roiConfigure = imageConfigure->roiConfigure->next;
            }
            HIAI_ENGINE_LOG(HIAI_ERROR, "sprintf_s fail, ret = %d", safeFuncRet);
            return;
        }
        outImageFp = fopen(fileName, "wb+");
        fwrite(imageConfigure->roiConfigure->outputConfigure.addr, 1,
            imageConfigure->roiConfigure->outputConfigure.bufferSize, outImageFp);
        fclose(outImageFp);
        imageConfigure->roiConfigure = imageConfigure->roiConfigure->next;
        imageCount++;
    }
    ret = DestroyDvppApi(pidvppapi);
    HIAI_DVPP_DFree(inBuffer);
    while (imageConfigure->roiConfigure != nullptr) {
        HIAI_DVPP_DFree(imageConfigure->roiConfigure->outputConfigure.addr);
        imageConfigure->roiConfigure = imageConfigure->roiConfigure->next;
    }
    return;
}

/*
 * 8K zoom function, 8129*8192 zoom to 4000*4000.
 */
void NewVpcTest4()
{
    uint32_t inWidthStride = 8192; // No need for 128 byte alignment
    uint32_t inHeightStride = 8192; // No need for 16 byte alignment
    uint32_t outWidthStride = 4000; // No need for 128 byte alignment
    uint32_t outHeightStride = 4000; // No need for 16 byte alignment
    uint32_t inBufferSize = inWidthStride * inHeightStride * 3 / 2;
    uint32_t outBufferSize = outWidthStride * outHeightStride * 3 / 2; // Construct dummy data
    uint8_t* inBuffer = (uint8_t*)HIAI_DVPP_DMalloc(inBufferSize); // Construct input image
    uint8_t* outBuffer = (uint8_t*)HIAI_DVPP_DMalloc(outBufferSize); // Construct output image

    FILE* fp = fopen("dvpp_vpc_8192x8192_nv12.yuv", "rb+");
    if (fp == nullptr) {
        HIAI_ENGINE_LOG(HIAI_ERROR, "fopen file failed");
        fclose(fp);
        return;
    }
    fread(inBuffer, 1, inBufferSize, fp);
    fclose(fp);
    std::shared_ptr<VpcUserImageConfigure> imageConfigure(new VpcUserImageConfigure);
    imageConfigure->bareDataAddr = inBuffer;
    imageConfigure->bareDataBufferSize = inBufferSize;
    imageConfigure->isCompressData = false;
    imageConfigure->widthStride = inWidthStride;
    imageConfigure->heightStride = inHeightStride;
    imageConfigure->inputFormat = INPUT_YUV420_SEMI_PLANNER_UV;
    imageConfigure->outputFormat = OUTPUT_YUV420SP_UV;
    imageConfigure->yuvSumEnable = false;
    imageConfigure->cmdListBufferAddr = nullptr;
    imageConfigure->cmdListBufferSize = 0;
    std::shared_ptr<VpcUserRoiConfigure> roiConfigure(new VpcUserRoiConfigure);
    roiConfigure->next = nullptr;
    VpcUserRoiInputConfigure* inputConfigure = &roiConfigure->inputConfigure; // Set the roi area
    inputConfigure->cropArea.leftOffset  = 0;
    inputConfigure->cropArea.rightOffset = inWidthStride - 1;
    inputConfigure->cropArea.upOffset = 0;
    inputConfigure->cropArea.downOffset  = inHeightStride - 1;
    VpcUserRoiOutputConfigure* outputConfigure = &roiConfigure->outputConfigure;
    outputConfigure->addr = outBuffer;
    outputConfigure->bufferSize = outBufferSize;
    outputConfigure->widthStride = outWidthStride;
    outputConfigure->heightStride = outHeightStride; // Set the map area
    outputConfigure->outputArea.leftOffset  = 0;
    outputConfigure->outputArea.rightOffset = outWidthStride - 1;
    outputConfigure->outputArea.upOffset = 0;
    outputConfigure->outputArea.downOffset  = outHeightStride - 1;
    imageConfigure->roiConfigure = roiConfigure.get();
    IDVPPAPI *pidvppapi = nullptr;
    int32_t ret = CreateDvppApi(pidvppapi);
    if (ret != 0) {
        HIAI_ENGINE_LOG(HIAI_CREATE_DVPP_ERROR, "create dvpp api fail.");
        HIAI_DVPP_DFree(inBuffer);
        HIAI_DVPP_DFree(outBuffer);
        return;
    }
    dvppapi_ctl_msg dvppApiCtlMsg;
    dvppApiCtlMsg.in = static_cast<void*>(imageConfigure.get());
    dvppApiCtlMsg.in_size = sizeof(VpcUserImageConfigure);
    ret = DvppCtl(pidvppapi, DVPP_CTL_VPC_PROC, &dvppApiCtlMsg);
    if (ret != 0) {
        HIAI_ENGINE_LOG(HIAI_VPC_CTL_ERROR, "call vpc dvppctl process faild!");
        ret = DestroyDvppApi(pidvppapi);
        return;
    } else {
        HIAI_ENGINE_LOG(HIAI_OK, "NewVpcTest4::call vpc dvppctl process success!");
    }

    FILE* outImageFp = fopen("NewVpcTest4Out.yuv", "wb+");
    fwrite(outBuffer, 1, outBufferSize, outImageFp);
    fclose(outImageFp);
    ret = DestroyDvppApi(pidvppapi);
    HIAI_DVPP_DFree(inBuffer);
    HIAI_DVPP_DFree(outBuffer);
    return;
}

/*
* vpc new interface test program
*/
void TEST_VPC_1() // new vpc interface
{
    NewVpcTest1(); // Original Image Zoom Case
    NewVpcTest2(); // Attach the test case.
    NewVpcTest3(); // Multi-frame test case in one figure
    NewVpcTest4(); // 8K zoom function, 8129*8192 zoom to 4000*4000
}

/*
* vdec test program to achieve h264/h265 video decoding function.
*/
void TEST_VDEC(int idx)
{
    char output_dir[20];
    int32_t safeFuncRet = memset_s(output_dir, sizeof(output_dir), 0, 20);
    if (safeFuncRet != EOK) {
        HIAI_ENGINE_LOG(HIAI_ERROR, "memset_s fail");
        return;
    }
    safeFuncRet = strncpy_s(output_dir, sizeof(output_dir), "output_dir", strlen("output_dir"));
    if (safeFuncRet != EOK) {
        HIAI_ENGINE_LOG(HIAI_ERROR, "strncpy_s fail");
        return;
    }
    if (access(output_dir, 0) == -1) {
        if (mkdir(output_dir, S_IREAD | S_IWRITE) < 0) {
            HIAI_ENGINE_LOG(HIAI_VDEC_CTL_ERROR, "create dir failed.");
            return;
        }
    }
    IDVPPAPI *pidvppapi = nullptr;
    int32_t ret = CreateVdecApi(pidvppapi, 0);
    if (ret != 0) {
        HIAI_ENGINE_LOG(HIAI_CREATE_DVPP_ERROR, "create dvpp api fail.");
        return;
    }
    if (pidvppapi != nullptr) {
        FILE* fp = fopen(in_file_name, "rb");
        if (fp == nullptr) {
            HIAI_ENGINE_LOG(HIAI_OPEN_FILE_ERROR, "open file: %s failed.", in_file_name);
            DestroyVdecApi(pidvppapi, 0);
            return;
        }
        fseek(fp, 0L, SEEK_END);
        int file_size = ftell(fp);
        fseek(fp, 0L, SEEK_SET);
        int rest_len = file_size;
        int len = file_size;

        vdec_in_msg vdec_msg;
        if (idx == 0) vdec_msg.call_back = FrameReturn;
	else if (idx == 1) vdec_msg.call_back = FrameReturn1;
	else if (idx == 2) vdec_msg.call_back = FrameReturn2;
	else if (idx == 3) vdec_msg.call_back = FrameReturn3;
	else if (idx == 4) vdec_msg.call_back = FrameReturn4;
	else if (idx == 5) vdec_msg.call_back = FrameReturn5;
	else if (idx == 6) vdec_msg.call_back = FrameReturn6;
	else if (idx == 7) vdec_msg.call_back = FrameReturn7;
	else if (idx == 8) vdec_msg.call_back = FrameReturn8;
	else if (idx == 9) vdec_msg.call_back = FrameReturn9;
	else if (idx == 10) vdec_msg.call_back = FrameReturn10;
	else if (idx == 11) vdec_msg.call_back = FrameReturn11;
	else if (idx == 12) vdec_msg.call_back = FrameReturn12;
	else if (idx == 13) vdec_msg.call_back = FrameReturn13;
	else if (idx == 14) vdec_msg.call_back = FrameReturn14;
	else if (idx == 15) vdec_msg.call_back = FrameReturn15;

        vdec_msg.hiai_data = nullptr;
        int32_t safeFuncRet = 0;
        if (g_format == 0) {
            safeFuncRet = memcpy_s(vdec_msg.video_format, sizeof(vdec_msg.video_format), "h264", 4);
            if (safeFuncRet != EOK) {
                HIAI_ENGINE_LOG(HIAI_ERROR, "memcpy_s fail");
                return;
            }
        } else {
            safeFuncRet = memcpy_s(vdec_msg.video_format, sizeof(vdec_msg.video_format), "h265", 4);
            if (safeFuncRet != EOK) {
                HIAI_ENGINE_LOG(HIAI_ERROR, "memcpy_s fail");
                return;
            }
        }
        vdec_msg.in_buffer = (char*)malloc(len);

        dvppapi_ctl_msg dvppApiCtlMsg;
        dvppApiCtlMsg.in = (void*)(&vdec_msg);
        dvppApiCtlMsg.in_size = sizeof(vdec_in_msg);

        while (rest_len > 0) {
            int read_len = fread(vdec_msg.in_buffer, 1, len, fp);
            HIAI_ENGINE_LOG("rest_len is %d,read len is %d.", rest_len, read_len);
            vdec_msg.in_buffer_size = read_len;
            fclose(fp);
            if (VdecCtl(pidvppapi, DVPP_CTL_VDEC_PROC, &dvppApiCtlMsg, 0) != 0) {
                HIAI_ENGINE_LOG(HIAI_VDEC_CTL_ERROR, "call dvppctl process faild!");
                free(vdec_msg.in_buffer);
                vdec_msg.in_buffer = nullptr;
                DestroyVdecApi(pidvppapi, 0);
                return;
            }
            HIAI_ENGINE_LOG(HIAI_OK, "call vdec process success!");
            rest_len = rest_len - len;
        }
        free(vdec_msg.in_buffer);
        vdec_msg.in_buffer = nullptr;
    } else {
        HIAI_ENGINE_LOG(HIAI_VDEC_CTL_ERROR, "pidvppapi is null!");
    }
    DestroyVdecApi(pidvppapi, 0);
    return;
}

std::string vencOutFileName("venc.bin");
std::shared_ptr<FILE> vencOutFile(nullptr);
void VencCallBackDumpFile(struct VencOutMsg* vencOutMsg, void* userData)
{
    if (vencOutFile.get() == nullptr) {
        HIAI_ENGINE_LOG(HIAI_VENC_CTL_ERROR, "get venc out file fail!");
        return;
    }
    fwrite(vencOutMsg->outputData, 1, vencOutMsg->outputDataSize, vencOutFile.get());
    fflush(vencOutFile.get());
}

/*
 * venc new interface to achieve venc basic functions.
 */
void TEST_VENC()
{
    std::shared_ptr<FILE> fpIn(fopen(in_file_name, "rb"), fclose);
    vencOutFile.reset(fopen(vencOutFileName.c_str(), "wb"), fclose);

    if (fpIn.get() == nullptr || vencOutFile.get() == nullptr) {
        HIAI_ENGINE_LOG(HIAI_OPEN_FILE_ERROR, "open open venc in/out file failed.");
        return;
    }

    fseek(fpIn.get(), 0, SEEK_END);
    uint32_t fileLen = ftell(fpIn.get());
    fseek(fpIn.get(), 0, SEEK_SET);

    struct VencConfig vencConfig;
    vencConfig.width = g_width;
    vencConfig.height = g_high;
    vencConfig.codingType = g_format;
    vencConfig.yuvStoreType = g_yuvStoreType;
    vencConfig.keyFrameInterval = 16;
    vencConfig.vencOutMsgCallBack = VencCallBackDumpFile;
    vencConfig.userData = nullptr;

    int32_t vencHandle = CreateVenc(&vencConfig);
    if (vencHandle == -1) {
        HIAI_ENGINE_LOG(HIAI_VENC_CTL_ERROR, "CreateVenc fail!");
        return;
    }

    // input 16 frames once
    uint32_t inDataLenMaxOnce = g_width * g_high * 3 / 2;
    std::shared_ptr<char> inBuffer(static_cast<char*>(malloc(inDataLenMaxOnce)), free);

    if (inBuffer.get() == nullptr) {
        HIAI_ENGINE_LOG(HIAI_OPEN_FILE_ERROR, "alloc input buffer failed");
        DestroyVenc(vencHandle);
        return;
    }

    uint32_t inDataUnhandledLen = fileLen;

    auto start = std::chrono::system_clock::now();
    uint32_t frameCount = 0;

    while (inDataUnhandledLen > 0) {
        uint32_t inDataLen     = inDataUnhandledLen;
        if (inDataUnhandledLen > inDataLenMaxOnce) {
            inDataLen          = inDataLenMaxOnce;
        }
        inDataUnhandledLen    -= inDataLen;
        uint32_t readLen       = fread(inBuffer.get(), 1, inDataLen, fpIn.get());
        if (readLen != inDataLen) {
            HIAI_ENGINE_LOG(HIAI_OPEN_FILE_ERROR, "error in read input file");
            DestroyVenc(vencHandle);
            return;
        }

        struct VencInMsg vencInMsg;
        vencInMsg.inputData = inBuffer.get();
        vencInMsg.inputDataSize = inDataLen;
        vencInMsg.keyFrameInterval = 16;
        vencInMsg.forceIFrame = 0;
        vencInMsg.eos = 0;

        if (RunVenc(vencHandle, &vencInMsg) == -1) {
            HIAI_ENGINE_LOG(HIAI_VENC_CTL_ERROR, "call video encode fail");
            break;
        }
        ++frameCount;
    }

    auto end = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    size_t timeCount = static_cast<size_t>(duration.count());
    HIAI_ENGINE_LOG("Total frame count: %u", frameCount);
    HIAI_ENGINE_LOG("Time cost: %lu us", timeCount);
    HIAI_ENGINE_LOG("FPS: %lf", frameCount * 1.0 / (timeCount / 1000000.0));

    DestroyVenc(vencHandle);
    return;
}

/*
 * test for jpege.
 */
void JpegeProcess()
{
    sJpegeIn  inData;
    sJpegeOut outData;

    inData.width         = g_width;
    inData.height        = g_high;
    inData.heightAligned = g_high; // no need to align
    inData.format        = (eEncodeFormat)g_format;
    inData.level         = 100;

    if (JPGENC_FORMAT_YUV420 == (inData.format & JPGENC_FORMAT_BIT)) {
        inData.stride  = ALIGN_UP(inData.width, 16);
        inData.bufSize = inData.stride * inData.heightAligned * 3 / 2;
    } else {
        inData.stride  = ALIGN_UP(inData.width * 2, 16);
        inData.bufSize = inData.stride * inData.heightAligned;
    }

    void* addrOrig = HIAI_DVPP_DMalloc(inData.bufSize);

    if (addrOrig == MAP_FAILED) {
        HIAI_ENGINE_LOG(HIAI_JPEGE_CTL_ERROR, "can not alloc input buffer");
        return;
    }
    inData.buf = reinterpret_cast<unsigned char*>(addrOrig);

    do {
        int err = 0;
        // load img file
        FILE* fpIn = fopen(in_file_name, "rb");
        if (fpIn == nullptr) {
            HIAI_ENGINE_LOG(HIAI_OPEN_FILE_ERROR, "can not open input file");
            break;
        }
        // only copy valid image data, other part is pending
        if (JPGENC_FORMAT_YUV420 == (inData.format & JPGENC_FORMAT_BIT)) {
            // for yuv422packed format, like uyvy / vyuy / yuyv / yvyu
            HIAI_ENGINE_LOG("input yuv 420 data");
            // for y data
            for (uint32_t j = 0; j < inData.height; j++) {
                fread(inData.buf + j * inData.stride, 1, inData.width, fpIn);
            }
            // for uv data
            for (uint32_t j = inData.heightAligned; j < inData.heightAligned + inData.height / 2; j++) {
                fread(inData.buf + j * inData.stride, 1, inData.width, fpIn);
            }
        } else {
            // for yuv420semi-planar format, like nv12 / nv21
            HIAI_ENGINE_LOG("input yuv 422 data");
            for (uint32_t j = 0; j < inData.height; j++) {
                fread(inData.buf + j * inData.stride, 1, inData.width * 2, fpIn );
            }

        }
        fclose(fpIn);
        if (err == -1) {
            break;
        }

        // call jpege process
        dvppapi_ctl_msg dvppApiCtlMsg;
        dvppApiCtlMsg.in = (void*)&inData;
        dvppApiCtlMsg.in_size = sizeof(inData);
        dvppApiCtlMsg.out = (void*)&outData;
        dvppApiCtlMsg.out_size = sizeof(outData);

        IDVPPAPI *pidvppapi = nullptr;
        CreateDvppApi(pidvppapi);
        if (pidvppapi == nullptr) {
            HIAI_ENGINE_LOG(HIAI_JPEGE_CTL_ERROR, "can not open dvppapi engine");
            break;
        }

        JpegeProcessBranch(pidvppapi, dvppApiCtlMsg, outData);

        DestroyDvppApi(pidvppapi);

    } while (0); // for resource inData.buf
    if (addrOrig != nullptr) {
        HIAI_DVPP_DFree(addrOrig);
    }
}

/*
 * only handle jpege process.
 */
void JpegeProcessBranch(IDVPPAPI*& pidvppapi, dvppapi_ctl_msg& dvppApiCtlMsg, sJpegeOut& outData)
{
    do {
        for (int i = 0;i < g_loop; i++) { // same picture loop test
            if (DvppCtl(pidvppapi, DVPP_CTL_JPEGE_PROC, &dvppApiCtlMsg)) {
                HIAI_ENGINE_LOG(HIAI_JPEGE_CTL_ERROR, "call jpeg encoder fail");
                break;
            }
            if (i < g_loop - 1) {
                outData.cbFree();
            }
        }
        stringstream outFile;
        outFile << out_file_name << "_t" << std::this_thread::get_id() << ".jpg";

        FILE* fpOut = fopen(outFile.str().c_str(), "wb");
        if (fpOut != nullptr) {
            fwrite(outData.jpgData, 1, outData.jpgSize, fpOut);
            fflush(fpOut);
            fclose(fpOut);
        } else {
            HIAI_ENGINE_LOG(HIAI_JPEGE_CTL_ERROR, "call not save result file %s ", outFile.str().c_str());
        }

        outData.cbFree();
        HIAI_ENGINE_LOG(HIAI_OK, "jpeg encode process completed");
    } while (0); // for resource pddvppapi
}

/*
 * jpege test program to achieve jpeg image encoding function.
 */
void TEST_JPEGE()
{
    if (g_threadNum < 1) {
        g_threadNum = 1;
    }

    vector<thread> threads;
    for (int i = 0; i < g_threadNum; i++) {
        threads.push_back(thread(JpegeProcess));
    }

    for (auto& t : threads) {
        t.join();
    }
}

bool IsHandleJpegeCtlSucc(dvppapi_ctl_msg dvppApiCtlMsg, unsigned char* tmpAddr, uint32_t size, sJpegeOut* outData)
{
    IDVPPAPI *pidvppapi = nullptr;
    CreateDvppApi(pidvppapi);
    if (pidvppapi == nullptr) {
        HIAI_ENGINE_LOG(HIAI_JPEGE_CTL_ERROR, "can not open dvppapi engine");
        return false;
    }

    do {
        for (int i = 0;i < g_loop; i++) {
            // Re-assign initial value here when looping
            ((sJpegeOut*)(dvppApiCtlMsg.out))->jpgData = tmpAddr;
            ((sJpegeOut*)(dvppApiCtlMsg.out))->jpgSize = size;
            HIAI_ENGINE_LOG("tmp Addr is %p, size is %u", tmpAddr, size);
            if (DvppCtl(pidvppapi, DVPP_CTL_JPEGE_PROC, &dvppApiCtlMsg)) {
                HIAI_ENGINE_LOG(HIAI_JPEGE_CTL_ERROR, "call jpeg encoder fail");
                break;
            }
        }

        stringstream outFile;
        outFile << out_file_name << "_t" << std::this_thread::get_id() << ".jpg";

        FILE* fpOut = fopen(outFile.str().c_str(), "wb");
        if (fpOut) {
            fwrite(outData->jpgData, 1, outData->jpgSize, fpOut);
            fflush(fpOut);
            fclose(fpOut);
        } else {
            HIAI_ENGINE_LOG(HIAI_JPEGE_CTL_ERROR, "call not save result file %s ", outFile.str());
        }

        HIAI_ENGINE_LOG(HIAI_OK, "jpeg encode process completed");
    } while (0); // for resource pddvppapi
    int32_t ret = DestroyDvppApi(pidvppapi);
    if (ret == -1) {
        return false;
    }
    return true;
}


/*
 * jpege test program, use specified memory.
 */
void TEST_JPEGE_CUSTOM_MEMORY()
{
    sJpegeIn  inData;
    sJpegeOut outData;

    inData.width         = g_width;
    inData.height        = g_high;
    inData.heightAligned = g_high; // no need to align
    inData.format        = (eEncodeFormat)g_format;
    inData.level         = 100;

    if (JPGENC_FORMAT_YUV420 == (inData.format & JPGENC_FORMAT_BIT)) {
        inData.stride  = ALIGN_UP(inData.width, 16);
        inData.bufSize = inData.stride * inData.heightAligned * 3 / 2;
    } else {
        inData.stride  = ALIGN_UP(inData.width * 2, 16);
        inData.bufSize = inData.stride * inData.heightAligned;
    }

    void* addrOrig = HIAI_DVPP_DMalloc(inData.bufSize);

    if (addrOrig == nullptr) {
        HIAI_ENGINE_LOG(HIAI_JPEGE_CTL_ERROR, "can not alloc input buffer");
        return;
    }
    inData.buf = reinterpret_cast<unsigned char*>(addrOrig);

    unsigned char* tmpAddr = nullptr;

    do {
        int err = 0;
        // load img file
        FILE* fpIn = fopen(in_file_name, "rb");
        if (nullptr == fpIn) {
            HIAI_ENGINE_LOG(HIAI_OPEN_FILE_ERROR, "can not open input file");
            break;
        }
        // only copy valid image data, other part is pending
        if (JPGENC_FORMAT_YUV420 == (inData.format & JPGENC_FORMAT_BIT)) {
            // for yuv422packed format, like uyvy / vyuy / yuyv / yvyu
            HIAI_ENGINE_LOG("input yuv 420 data");
            // for y data
            for (uint32_t j = 0; j < inData.height; j++) {
                fread(inData.buf + j * inData.stride, 1, inData.width, fpIn);
            }
            // for uv data
            for (uint32_t j = inData.heightAligned; j < inData.heightAligned + inData.height / 2; j++) {
                fread(inData.buf + j * inData.stride, 1, inData.width, fpIn);
            }
        } else { // for yuv420semi-planar format, like nv12 / nv21
            HIAI_ENGINE_LOG("input yuv 422 data");
            for (uint32_t j = 0; j < inData.height; j++) {
                fread(inData.buf + j * inData.stride, 1, inData.width * 2, fpIn);
            }
        }
        fclose(fpIn);
        if (-1 == err) {
            break;
        }

        int32_t ret = DvppGetOutParameter((void*)(&inData), (void*)(&outData), GET_JPEGE_OUT_PARAMETER);
        // 此处获得的jpgSize为估算值，实际数据长度可能要小于这个值
        // 最终jpgSize大小，以调用DvppCtl接口以后，此字段才为真正的编码后的jpgSize大小
        HIAI_ENGINE_LOG("outdata size is %d", outData.jpgSize);
        tmpAddr = (unsigned char*)HIAI_DVPP_DMalloc(outData.jpgSize);
        uint32_t size = outData.jpgSize;

        // call jpege process
        dvppapi_ctl_msg dvppApiCtlMsg;
        dvppApiCtlMsg.in = (void *)&inData;
        dvppApiCtlMsg.in_size = sizeof(inData);
        dvppApiCtlMsg.out = (void *)&outData;
        dvppApiCtlMsg.out_size = sizeof(outData);

        if(!IsHandleJpegeCtlSucc(dvppApiCtlMsg, tmpAddr, size, &outData)) {
            break;
        }
    } while (0); // for resource inData.buf
    if (addrOrig != nullptr) {
        HIAI_DVPP_DFree(addrOrig);
        addrOrig = nullptr;
    }
    if (tmpAddr != nullptr) { // 注意：释放内存时需要使用tmpAddr，因为outData.jpgData在JPEGE处理后会有地址偏移
        HIAI_DVPP_DFree(tmpAddr);
        tmpAddr = nullptr;
    }
}


void WriteJpegdProcessResultToFile(struct jpegd_yuv_data_info* jpegdOutData)
{
    stringstream outFile;
    uint32_t width  = ALIGN_UP(jpegdOutData->img_width, 2);
    uint32_t height = ALIGN_UP(jpegdOutData->img_height, 2);

    outFile << "dvppapi_jpegd_w" << width
            << "_h" << height
            << "_f" << jpegdOutData->out_format
            << "_t" << std::this_thread::get_id() << ".yuv";

    uint32_t uvWidth        = width;
    uint32_t uvWidthAligned = jpegdOutData->img_width_aligned;
    uint32_t uvHeight       = height / 2;

    if (jpegdOutData->out_format == DVPP_JPEG_DECODE_OUT_YUV422_H2V1) {
        uvHeight            = height;
    }

    if (jpegdOutData->out_format == DVPP_JPEG_DECODE_OUT_YUV444) {
        uvWidth             = width * 2;
        uvWidthAligned      = jpegdOutData->img_width_aligned * 2;
        uvHeight            = height;
    }

    FILE* fpOut = fopen(outFile.str().c_str(), "wb");
    if (fpOut) {
        for (uint32_t j = 0; j < height; j++) {
            fwrite(jpegdOutData->yuv_data + j * jpegdOutData->img_width_aligned, 1, width, fpOut);
        }
        auto uvStart = jpegdOutData->yuv_data + jpegdOutData->img_height_aligned * jpegdOutData->img_width_aligned;
        for (uint32_t j = 0; j < uvHeight; j++) {
            fwrite(uvStart + j * uvWidthAligned, 1, uvWidth, fpOut);
        }
        fflush(fpOut);
        fclose(fpOut);
    } else {
        HIAI_ENGINE_LOG(HIAI_OPEN_FILE_ERROR, "can not open output file %s ", outFile.str());
    }
}

/*
 * only handle jpegd process.
 */
void JpegdProcess()
{
    struct jpegd_raw_data_info jpegdInData;
    struct jpegd_yuv_data_info jpegdOutData;

    if (g_rank) {
        jpegdInData.IsYUV420Need = false;
    }

    FILE *fpIn = fopen(in_file_name, "rb");
    if (fpIn == nullptr) {
        HIAI_ENGINE_LOG(HIAI_OPEN_FILE_ERROR, "can not open file %s.", in_file_name);
        return;
    }

    do { // for resource fpIn
        fseek(fpIn, 0, SEEK_END);
        // the buf len should 8 byte larger, the driver asked
        uint32_t fileLen   = ftell(fpIn);
        jpegdInData.jpeg_data_size = fileLen + 8;
        fseek(fpIn, 0, SEEK_SET);

        void* addrOrig = HIAI_DVPP_DMalloc(jpegdInData.jpeg_data_size);
        if (addrOrig == nullptr) {
            HIAI_ENGINE_LOG(HIAI_JPEGD_CTL_ERROR, "can not alloc input buffer");
            break;
        }

        jpegdInData.jpeg_data = reinterpret_cast<unsigned char*>(addrOrig);

        do { // for resource inBuf
            fread(jpegdInData.jpeg_data, 1, fileLen, fpIn);
            dvppapi_ctl_msg dvppApiCtlMsg;
            dvppApiCtlMsg.in = (void*)&jpegdInData;
            dvppApiCtlMsg.in_size = sizeof(jpegdInData);
            dvppApiCtlMsg.out = (void*)&jpegdOutData;
            dvppApiCtlMsg.out_size = sizeof(jpegdOutData);

            IDVPPAPI *pidvppapi = nullptr;
            CreateDvppApi(pidvppapi);

            if (pidvppapi != nullptr) {
                for (int i = 0;i < g_loop; i++) { // same picture loop test
                    if (DvppCtl(pidvppapi, DVPP_CTL_JPEGD_PROC, &dvppApiCtlMsg) != 0) {
                        HIAI_ENGINE_LOG(HIAI_JPEGD_CTL_ERROR, "call dvppctl process failed");
                        DestroyDvppApi(pidvppapi);
                        break;
                    }
                    if (i < g_loop - 1) {
                        jpegdOutData.cbFree();
                    }
                }
                DestroyDvppApi(pidvppapi);
            } else {
                HIAI_ENGINE_LOG(HIAI_CREATE_DVPP_ERROR, "can not create dvpp api");
                break;
            }

            WriteJpegdProcessResultToFile(&jpegdOutData);
            jpegdOutData.cbFree();
            jpegdOutData.yuv_data = nullptr;

        } while (0); // for resource inBuf

        if (addrOrig != nullptr) {
            HIAI_DVPP_DFree(addrOrig);
            addrOrig = nullptr;
        }

    } while (0); // for resource fpIn
    fclose(fpIn);
}

/*
 * jpegd test program to achieve jpeg image decoding.
 */
void TEST_JPEGD()
{
    if (g_threadNum < 1) {
        g_threadNum = 1;
    }

    vector<thread> threads;

    for (int i = 0; i < g_threadNum; i++) {
        threads.push_back(thread(JpegdProcess));
    }

    for (auto& t : threads) {
        t.join();
    }
}


void WriteTestJpegdResultToFile(struct JpegdOut* jpegdOutData)
{
    uint32_t width  = ALIGN_UP(jpegdOutData->imgWidth, 2);
    uint32_t height = ALIGN_UP(jpegdOutData->imgHeight, 2);
    stringstream outFile;
    outFile << "dvppapi_jpegd_w" << width
            << "_h" << height
            << "_f" << jpegdOutData->outFormat
            << "_t" << std::this_thread::get_id() << ".yuv";

    uint32_t uvWidth        = width;
    uint32_t uvWidthAligned = jpegdOutData->imgWidthAligned;
    uint32_t uvHeight       = height / 2;

    if (jpegdOutData->outFormat == DVPP_JPEG_DECODE_OUT_YUV422_H2V1) {
        uvHeight            = height;
    }

    if (jpegdOutData->outFormat == DVPP_JPEG_DECODE_OUT_YUV444) {
        uvWidth             = width * 2;
        uvWidthAligned      = jpegdOutData->imgWidthAligned * 2;
        uvHeight            = height;
    }

    FILE* fpOut = fopen(outFile.str().c_str(), "wb");
    if (fpOut != nullptr) {
        for (uint32_t j = 0; j < height; j++) {
            fwrite(jpegdOutData->yuvData + j * jpegdOutData->imgWidthAligned, 1, width, fpOut);
        }
        auto uvStart = jpegdOutData->yuvData + jpegdOutData->imgWidthAligned * jpegdOutData->imgHeightAligned;
        for (uint32_t j = 0; j < uvHeight; j++) {
            fwrite(uvStart + j * uvWidthAligned, 1, uvWidth, fpOut);
        }
        fflush(fpOut);
        fclose(fpOut);
    } else {
        HIAI_ENGINE_LOG(HIAI_OPEN_FILE_ERROR, "can not open output file %s ", outFile.str());
    }
}


/*
* jpegd test program, user specified memory.
*/
void TEST_JPEGD_CUSTOM_MEMORY()
{
    struct JpegdIn jpegdInData;
    struct JpegdOut jpegdOutData;

    if (g_rank) {
        jpegdInData.isYUV420Need = false;
    }

    FILE *fpIn = fopen(in_file_name, "rb");
    if (fpIn == nullptr) {
        HIAI_ENGINE_LOG(HIAI_OPEN_FILE_ERROR, "can not open file %s.", in_file_name);
        return;
    }

    do { // for resource fpIn
        fseek(fpIn, 0, SEEK_END);
        // the buf len should 8 byte larger, the driver asked
        uint32_t fileLen   = ftell(fpIn);
        jpegdInData.jpegDataSize = fileLen + 8;
        fseek(fpIn, 0, SEEK_SET);

        void* addrOrig = HIAI_DVPP_DMalloc(jpegdInData.jpegDataSize);
        if (addrOrig == nullptr) {
            HIAI_ENGINE_LOG(HIAI_JPEGD_CTL_ERROR, "can not alloc input buffer");
            break;
        }

        jpegdInData.jpegData = reinterpret_cast<unsigned char*>(addrOrig);
        do { // for resource inBuf
            fread(jpegdInData.jpegData, 1, fileLen, fpIn);
            fclose(fpIn);
            int32_t ret = DvppGetOutParameter((void*)(&jpegdInData), (void*)(&jpegdOutData), GET_JPEGD_OUT_PARAMETER);
            HIAI_ENGINE_LOG("jpegd out size is %d", jpegdOutData.yuvDataSize);
            jpegdOutData.yuvData = (unsigned char*)HIAI_DVPP_DMalloc(jpegdOutData.yuvDataSize);
            HIAI_ENGINE_LOG("jpegdOutData.yuvData is %p", jpegdOutData.yuvData);

            dvppapi_ctl_msg dvppApiCtlMsg;
            dvppApiCtlMsg.in = (void *)&jpegdInData;
            dvppApiCtlMsg.in_size = sizeof(jpegdInData);
            dvppApiCtlMsg.out = (void *)&jpegdOutData;
            dvppApiCtlMsg.out_size = sizeof(jpegdOutData);

            IDVPPAPI *pidvppapi = nullptr;
            CreateDvppApi(pidvppapi);

            if (pidvppapi != nullptr) {
                for (int i = 0;i < g_loop; i++) {
                    if (0 != DvppCtl(pidvppapi, DVPP_CTL_JPEGD_PROC, &dvppApiCtlMsg)) {
                        HIAI_ENGINE_LOG(HIAI_JPEGD_CTL_ERROR, "call dvppctl process failed");
                        DestroyDvppApi(pidvppapi);
                        break;
                    }
                }
                DestroyDvppApi(pidvppapi);
            } else {
                HIAI_ENGINE_LOG(HIAI_CREATE_DVPP_ERROR, "can not create dvpp api");
                break;
            }
            WriteTestJpegdResultToFile(&jpegdOutData);
        } while (0); // for resource inBuf

        if (addrOrig != nullptr) {
            HIAI_DVPP_DFree(addrOrig);
            addrOrig = nullptr;
        }
        if (jpegdOutData.yuvData != nullptr) {
            HIAI_DVPP_DFree(jpegdOutData.yuvData);
            jpegdOutData.yuvData = nullptr;
        }
    } while (0); // for resource fpIn
}

/*
 * pngd test program to achieve png image decoding.
 */
void TEST_PNGD()
{
    // load test file
    FILE* fpIn = fopen(in_file_name, "rb");
    if (fpIn == nullptr) {
        HIAI_ENGINE_LOG(HIAI_OPEN_FILE_ERROR, "can not open file %s.", in_file_name);
        return;
    }

    fseek(fpIn, 0, SEEK_END);
    uint32_t fileLen = ftell(fpIn);
    fseek(fpIn, 0, SEEK_SET);

    void* inbuf = HIAI_DVPP_DMalloc(fileLen);
    if (inbuf == nullptr) {
        HIAI_ENGINE_LOG(HIAI_PNGD_CTL_ERROR, "can not alloc memmory");
        fclose(fpIn);
        return;
    } else {
        fread(inbuf, 1, fileLen, fpIn);
    }

    fclose(fpIn);

    // prepare msg
    struct PngInputInfoAPI inputPngData;
    inputPngData.inputData = inbuf;
    inputPngData.inputSize = fileLen;
    if (g_transform == 1) {
        inputPngData.transformFlag = 1; // RGBA -> RGB
    } else {
        inputPngData.transformFlag = 0;
    }

    struct PngOutputInfoAPI outputPngData;

    dvppapi_ctl_msg dvppApiCtlMsg;
    dvppApiCtlMsg.in = (void*)(&inputPngData);
    dvppApiCtlMsg.in_size = sizeof(struct PngInputInfoAPI);
    dvppApiCtlMsg.out = (void*)(&outputPngData);
    dvppApiCtlMsg.out_size = sizeof(struct PngOutputInfoAPI);

    // use interface
    IDVPPAPI *pidvppapi = nullptr;
    CreateDvppApi(pidvppapi);

    if (pidvppapi != nullptr) {
        for (int i = 0; i < g_loop; i++) {
            if (DvppCtl(pidvppapi, DVPP_CTL_PNGD_PROC, &dvppApiCtlMsg) == -1) {
                HIAI_ENGINE_LOG(HIAI_PNGD_CTL_ERROR, "call dvppctl process failed");
                DestroyDvppApi(pidvppapi);
                return;
            }
        }
    } else {
        HIAI_ENGINE_LOG(HIAI_PNGD_CTL_ERROR, "can not get dvpp api");
    }

    DestroyDvppApi(pidvppapi);

    HIAI_ENGINE_LOG("output result");

    char* pAddr = (char*)outputPngData.outputData;
    FILE* fpOut = fopen(out_file_name, "wb");

    int size = outputPngData.width * 4;
    if (outputPngData.format == PNGD_RGB_FORMAT_NUM) {
        size = outputPngData.width * 3;
    } else if (outputPngData.format == PNGD_RGBA_FORMAT_NUM) {
        size = outputPngData.width * 4;
    }
    // copy valid image data from every line.
    for (int i = 0; i < outputPngData.high; i++) {
        fwrite(pAddr + (int)(i * outputPngData.widthAlign), size, 1, fpOut);
    }

    fclose(fpOut);
    HIAI_DVPP_DFree(inbuf);
}

/*
 * pngd test program, user specified memory.
 */
void TEST_PNGD_CUSTOM_MEMORY()
{
    FILE* fpIn = fopen(in_file_name, "rb");
    if (nullptr == fpIn) {
        HIAI_ENGINE_LOG(HIAI_OPEN_FILE_ERROR, "can not open file %s.", in_file_name);
        return;
    }

    fseek(fpIn, 0, SEEK_END);
    uint32_t fileLen = ftell(fpIn);
    fseek(fpIn, 0, SEEK_SET);
    void* inbuf = HIAI_DVPP_DMalloc(fileLen);

    // prepare msg
    struct PngInputInfoAPI inputPngData; // input data
    inputPngData.inputData = inbuf; // input png data
    inputPngData.inputSize = fileLen; // the size of png data

    HIAI_ENGINE_LOG("inputPngData.address: %p", inputPngData.address);
    HIAI_ENGINE_LOG("inputPngData.size: %u", inputPngData.size);
    HIAI_ENGINE_LOG("inputPngData.inputData: %p", inputPngData.inputData);
    HIAI_ENGINE_LOG("inputPngData.inputSize: %u", inputPngData.inputSize);

    fread(inputPngData.inputData, 1, fileLen, fpIn);
    fclose(fpIn);

    if (g_transform == 1) { // Whether format conversion
        inputPngData.transformFlag = 1; // RGBA -> RGB
    } else {
        inputPngData.transformFlag = 0;
    }

    struct PngOutputInfoAPI outputPngData; // output data
    int32_t ret = DvppGetOutParameter((void*)(&inputPngData), (void*)(&outputPngData), GET_PNGD_OUT_PARAMETER);
    HIAI_ENGINE_LOG("pngd out size is  %d", outputPngData.size);
    outputPngData.address = HIAI_DVPP_DMalloc(outputPngData.size);

    dvppapi_ctl_msg dvppApiCtlMsg; // call the interface msg
    dvppApiCtlMsg.in = (void*)(&inputPngData);
    dvppApiCtlMsg.in_size = sizeof(struct PngInputInfoAPI);
    dvppApiCtlMsg.out = (void*)(&outputPngData);
    dvppApiCtlMsg.out_size = sizeof(struct PngOutputInfoAPI);

    // use interface
    IDVPPAPI *pidvppapi = nullptr;
    CreateDvppApi(pidvppapi); // create dvppapi

    if (pidvppapi != nullptr) { // use DvppCtl interface to handle DVPP_CTL_PNGD_PROC
        for (int i = 0; i < g_loop; i++) {
            if (-1 == DvppCtl(pidvppapi, DVPP_CTL_PNGD_PROC, &dvppApiCtlMsg)) {
                HIAI_ENGINE_LOG(HIAI_PNGD_CTL_ERROR, "call dvppctl process failed");
                HIAI_DVPP_DFree(inbuf);
                HIAI_DVPP_DFree(outputPngData.address);
                outputPngData.address = nullptr;
                DestroyDvppApi(pidvppapi); // destory dvppapi
                return;
            }
        }
    } else {
        HIAI_ENGINE_LOG(HIAI_PNGD_CTL_ERROR, "can not get dvpp api");
    }

    DestroyDvppApi(pidvppapi);

    HIAI_ENGINE_LOG("output result");

    char* pAddr = (char*)outputPngData.outputData;
    FILE* fpOut = fopen(out_file_name, "wb");

    int size = outputPngData.width * 4;
    if (outputPngData.format == PNGD_RGB_FORMAT_NUM) {
        size = outputPngData.width * 3;
    } else if (outputPngData.format == PNGD_RGBA_FORMAT_NUM) {
        size = outputPngData.width * 4;
    }
    // copy valid image data from every line.
    for (int i = 0; i < outputPngData.high; i++) {
        fwrite(pAddr + (int)(i * outputPngData.widthAlign), size, 1, fpOut);
    }

    fclose(fpOut);
    HIAI_DVPP_DFree(inbuf);
    HIAI_DVPP_DFree(outputPngData.address);
    outputPngData.address = nullptr;
}

int IsHandleSerialTestSucc(struct jpegd_raw_data_info* jpegdInData, struct jpegd_yuv_data_info* jpegdOutData, uint32_t fileLen, FILE *fpIn)
{
    do { // for resource inBuf
        fread(jpegdInData->jpeg_data, 1, fileLen, fpIn);
        dvppapi_ctl_msg dvppApiCtlMsg;
        dvppApiCtlMsg.in = (void*)jpegdInData;
        dvppApiCtlMsg.in_size = sizeof(*jpegdInData);
        dvppApiCtlMsg.out = (void*)jpegdOutData;
        dvppApiCtlMsg.out_size = sizeof(*jpegdOutData);

        IDVPPAPI *pidvppapi = nullptr;
        CreateDvppApi(pidvppapi);

        if (pidvppapi != nullptr) {
            if (DvppCtl(pidvppapi, DVPP_CTL_JPEGD_PROC, &dvppApiCtlMsg) != 0) {
                HIAI_ENGINE_LOG(HIAI_ERROR, "call dvppctl process failed");
                DestroyDvppApi(pidvppapi);
                return -1;
            }
            DestroyDvppApi(pidvppapi);
        } else {
            HIAI_ENGINE_LOG(HIAI_CREATE_DVPP_ERROR, "can not create dvpp api");
            return -1;
        }
    } while (0); // for resource inBuf
    return 0;
}

/*
 * jpegd+vpc+jpege module serial test program by old interface.
 */
void TEST_JPEGD_VPC_JPEGE()
{
    int err = 0;

    struct jpegd_raw_data_info jpegdInData;
    struct jpegd_yuv_data_info jpegdOutData;
    FILE *fpIn = fopen(in_file_name, "rb");
    if (fpIn == nullptr) {
        HIAI_ENGINE_LOG(HIAI_OPEN_FILE_ERROR, "can not open file %s.", in_file_name);
        return;
    }

    do { // for resource fpIn
        fseek(fpIn, 0, SEEK_END);
        uint32_t fileLen   = ftell(fpIn);
        jpegdInData.jpeg_data_size = fileLen + 8;
        fseek(fpIn, 0, SEEK_SET);
        unsigned char* addrOrig = (unsigned char*)HIAI_DVPP_DMalloc(jpegdInData.jpeg_data_size);
        if (addrOrig == nullptr) {
            err = -1;
            HIAI_ENGINE_LOG(HIAI_ERROR, "can not alloc input buffer");
            break;
        }

        jpegdInData.jpeg_data = addrOrig;
        err = IsHandleSerialTestSucc(&jpegdInData, &jpegdOutData, fileLen, fpIn);
        if (addrOrig != nullptr) {
            HIAI_DVPP_DFree(addrOrig);
        }
    } while (0); // for resource fpIn
    fclose(fpIn);

    if (err == -1) {
        return;
    }
    HIAI_ENGINE_LOG("jpegd process success.");
    // vpc proccess
    dvppapi_ctl_msg vpcDvppApiCtlMsg;
    vpc_in_msg vpcInMsg;
    vpcInMsg.format = 0;
    vpcInMsg.rank = 1;
    vpcInMsg.width = jpegdOutData.img_width_aligned;
    vpcInMsg.high = jpegdOutData.img_height_aligned;
    vpcInMsg.stride = jpegdOutData.img_width_aligned;

    // jpeg  : 1024 900
    // jpegd : 1024 912, 1024 900
    // vpc   : [0, 1024-1, 0, 900-1]  [0, 1000-1, 0, 800-1]
    // hisi frame buffer compress  :

    vpcInMsg.hmax = (g_hMax < jpegdOutData.img_width ? g_hMax : jpegdOutData.img_width);
    vpcInMsg.hmin = (g_hMin > 0 ? g_hMin : 0);
    vpcInMsg.vmax = (g_vMax < jpegdOutData.img_height ? g_vMax : jpegdOutData.img_height);
    vpcInMsg.vmin = (g_vMin > 0 ? g_vMin : 0);

    HIAI_ENGINE_LOG("JPEGD ==> VPC, with: ");
    HIAI_ENGINE_LOG("width & stride %d ", vpcInMsg.width);
    HIAI_ENGINE_LOG("height %d ", vpcInMsg.high);
    HIAI_ENGINE_LOG("hmax %d hmin %d ", vpcInMsg.hmax, vpcInMsg.hmin);
    HIAI_ENGINE_LOG("vmax %d vmin %d ", vpcInMsg.vmax, vpcInMsg.vmin);

    vpcInMsg.vinc = (double)g_outHigh / (g_vMax - g_vMin + 1);
    vpcInMsg.hinc = (double)g_outWidth / (g_hMax - g_hMin + 1);

    TEST_JPEGD_VPC_JPEGE_Branch(vpcDvppApiCtlMsg, jpegdOutData, vpcInMsg);
}

void TEST_JPEGD_VPC_JPEGE_Branch(dvppapi_ctl_msg& vpcDvppApiCtlMsg, jpegd_yuv_data_info& jpegdOutData,
    vpc_in_msg& vpcInMsg)
{
    // alloc in and out buffer
    shared_ptr<AutoBuffer> auto_out_buffer = make_shared<AutoBuffer>();
    vpcInMsg.in_buffer = (char*)jpegdOutData.yuv_data;
    vpcInMsg.in_buffer_size = jpegdOutData.yuv_data_size;
    vpcInMsg.auto_out_buffer_1 = auto_out_buffer;
    vpcDvppApiCtlMsg.in = (void*)(&vpcInMsg);
    vpcDvppApiCtlMsg.in_size = sizeof(vpc_in_msg);
    IDVPPAPI *pVpcIdvppapi = nullptr;
    CreateDvppApi(pVpcIdvppapi);
    if (pVpcIdvppapi != nullptr) {
        if (DvppCtl(pVpcIdvppapi, DVPP_CTL_VPC_PROC, &vpcDvppApiCtlMsg) != 0) {
            HIAI_ENGINE_LOG(HIAI_VPC_CTL_ERROR, "call dvppctl vpc process faild!");
            DestroyDvppApi(pVpcIdvppapi);
            return;
        }
    } else {
        HIAI_ENGINE_LOG(HIAI_VPC_CTL_ERROR, "pVpcIdvppapi is null!");
    }

    DestroyDvppApi(pVpcIdvppapi);
    HIAI_ENGINE_LOG("vpc process success.");

    jpegdOutData.cbFree();

    // jpege proccess
    sJpegeIn inData;
    sJpegeOut outData;

    inData.format        = JPGENC_FORMAT_NV21;
    inData.width         = g_outWidth;
    inData.height        = g_outHigh;
    inData.heightAligned = ALIGN_UP(g_outHigh, 16);
    inData.stride        = ALIGN_UP(g_outWidth, 128);
    inData.buf           = (unsigned char*)auto_out_buffer->getBuffer();
    inData.bufSize       = inData.stride * inData.heightAligned * 3 / 2;
    inData.level         = 100;

    HIAI_ENGINE_LOG("VPC ==> JPEGE, with: ");
    HIAI_ENGINE_LOG("width  %d ", inData.width);
    HIAI_ENGINE_LOG("height %d ", inData.height);
    HIAI_ENGINE_LOG("stride  %d ", inData.stride);
    HIAI_ENGINE_LOG("heightAligned %d ", inData.heightAligned);

    dvppapi_ctl_msg dvppApiCtlMsg;
    dvppApiCtlMsg.in = (void *)&inData;
    dvppApiCtlMsg.in_size = sizeof(inData);
    dvppApiCtlMsg.out = (void *)&outData;
    dvppApiCtlMsg.out_size = sizeof(outData);
    IDVPPAPI *pidvppapi = nullptr;
    CreateDvppApi(pidvppapi);
    do {
        if (DvppCtl(pidvppapi, DVPP_CTL_JPEGE_PROC, &dvppApiCtlMsg) != 0) {
            HIAI_ENGINE_LOG(HIAI_JPEGE_CTL_ERROR, "call jpeg encoder fail");
            break;
        }

        string outFl = string(out_file_name) + ".jpg";

        FILE* fpOut = fopen(outFl.c_str(), "wb");
        if (fpOut != nullptr) {
            fwrite(outData.jpgData, 1, outData.jpgSize, fpOut);
            fflush(fpOut);
            fclose(fpOut);
        } else {
            HIAI_ENGINE_LOG(HIAI_ERROR, "can not save result file %s", out_file_name);
        }
        outData.cbFree();
    }while(0); // for resource pddvppapi
    DestroyDvppApi(pidvppapi);
    HIAI_ENGINE_LOG("jpege process success.");
}

/*
 * jpegd+vpc+jpege module serial test program by new interface(recommend).
 */
void TEST_JPEGD_VPC_JPEGE_NEW()
{
    struct JpegdIn jpegdInData;
    struct JpegdOut jpegdOutData;

    FILE *fpIn = fopen(in_file_name, "rb");
    if (fpIn == nullptr) {
        HIAI_ENGINE_LOG(HIAI_OPEN_FILE_ERROR, "can not open file %s.", in_file_name);
        return;
    }

    fseek(fpIn, 0, SEEK_END);
    // the buf len should 8 byte larger, the driver asked
    uint32_t fileLen   = ftell(fpIn);
    jpegdInData.jpegDataSize = fileLen + 8;
    fseek(fpIn, 0, SEEK_SET);

    void* jpegData = HIAI_DVPP_DMalloc(jpegdInData.jpegDataSize);
    if (jpegData == nullptr) {
        HIAI_ENGINE_LOG(HIAI_JPEGD_CTL_ERROR, "can not alloc input buffer");
        return;
    }

    jpegdInData.jpegData = reinterpret_cast<unsigned char*>(jpegData);
    IDVPPAPI *pJpegdIDvppApi = nullptr;

    do { // for resource inBuf
        fread(jpegdInData.jpegData, 1, fileLen, fpIn);
        fclose(fpIn);
        int32_t ret = DvppGetOutParameter((void*)(&jpegdInData), (void*)(&jpegdOutData), GET_JPEGD_OUT_PARAMETER);
        HIAI_ENGINE_LOG("jpegd out size is %d", jpegdOutData.yuvDataSize);
        jpegdOutData.yuvData = (unsigned char*)HIAI_DVPP_DMalloc(jpegdOutData.yuvDataSize);
        HIAI_ENGINE_LOG("jpegdOutData.yuvData is %p", jpegdOutData.yuvData);

        dvppapi_ctl_msg dvppJpegdCtlMsg;
        dvppJpegdCtlMsg.in = (void *)&jpegdInData;
        dvppJpegdCtlMsg.in_size = sizeof(jpegdInData);
        dvppJpegdCtlMsg.out = (void *)&jpegdOutData;
        dvppJpegdCtlMsg.out_size = sizeof(jpegdOutData);

        CreateDvppApi(pJpegdIDvppApi);

        if (pJpegdIDvppApi != nullptr) {
            if (DvppCtl(pJpegdIDvppApi, DVPP_CTL_JPEGD_PROC, &dvppJpegdCtlMsg) != 0) {
                HIAI_ENGINE_LOG(HIAI_JPEGD_CTL_ERROR, "call dvppctl process failed");
                break;
            }
        } else {
            HIAI_ENGINE_LOG(HIAI_CREATE_DVPP_ERROR, "can not create dvpp api");
            break;
        }
    } while (0); // for resource inBuf

    if (jpegData != nullptr) {
        HIAI_DVPP_DFree(jpegData);
        jpegData = nullptr;
    }
    DestroyDvppApi(pJpegdIDvppApi);
    HIAI_ENGINE_LOG("jpegd process success.");

    // call vpc process
    int32_t inWidthStride = jpegdOutData.imgWidthAligned;
    int32_t inHighStride = jpegdOutData.imgHeightAligned;
    uint32_t vpcOutBufferSize = g_outWidthStride * g_outHighStride * 3 / 2;
    uint8_t* vpcOutBuffer = static_cast<uint8_t*>(HIAI_DVPP_DMalloc(vpcOutBufferSize)); // 申请输出buffer

    std::shared_ptr<VpcUserImageConfigure> imageConfigure(new VpcUserImageConfigure);
    imageConfigure->bareDataAddr = reinterpret_cast<uint8_t*>(jpegdOutData.yuvData);
    imageConfigure->bareDataBufferSize = jpegdOutData.yuvDataSize;
    imageConfigure->widthStride = inWidthStride;
    imageConfigure->heightStride = inHighStride;
    HIAI_ENGINE_LOG("inWidthStride = %u", imageConfigure->widthStride);
    HIAI_ENGINE_LOG("inHighStride = %u", imageConfigure->heightStride);

    imageConfigure->inputFormat = INPUT_YUV420_SEMI_PLANNER_VU;
    imageConfigure->outputFormat = OUTPUT_YUV420SP_UV;
    HIAI_ENGINE_LOG("inFormat= %u", imageConfigure->inputFormat);
    HIAI_ENGINE_LOG("outFormat= %u", imageConfigure->outputFormat);
    imageConfigure->yuvSumEnable = false;
    imageConfigure->cmdListBufferAddr = nullptr;
    imageConfigure->cmdListBufferSize = 0;
    std::shared_ptr<VpcUserRoiConfigure> roiConfigure(new VpcUserRoiConfigure);
    roiConfigure->next = nullptr;
    VpcUserRoiInputConfigure* inputConfigure = &roiConfigure->inputConfigure;
    // Set the roi area, the top left corner coordinate of the roi area [0,0].
    inputConfigure->cropArea.leftOffset  = g_hMin;
    inputConfigure->cropArea.rightOffset = g_hMax;
    inputConfigure->cropArea.upOffset    = g_vMin;
    inputConfigure->cropArea.downOffset  = g_vMax;
    HIAI_ENGINE_LOG("cropArea.leftOffset = %u", inputConfigure->cropArea.leftOffset);
    HIAI_ENGINE_LOG("cropArea.rightOffset = %u", inputConfigure->cropArea.rightOffset);
    HIAI_ENGINE_LOG("cropArea.upOffset = %u", inputConfigure->cropArea.upOffset);
    HIAI_ENGINE_LOG("cropArea.downOffset = %u", inputConfigure->cropArea.downOffset);

    VpcUserRoiOutputConfigure* outputConfigure = &roiConfigure->outputConfigure;
    outputConfigure->addr = vpcOutBuffer;
    outputConfigure->bufferSize = vpcOutBufferSize;
    outputConfigure->widthStride = g_outWidthStride;
    outputConfigure->heightStride = g_outHighStride;
    HIAI_ENGINE_LOG("outputConfigure->widthStride = %u", outputConfigure->widthStride);
    HIAI_ENGINE_LOG("outputConfigure->heightStride = %u",  outputConfigure->heightStride);
    // Set the texture area, the top left corner of the texture area [0,0].
    outputConfigure->outputArea.leftOffset  = g_outLeftOffset;
    outputConfigure->outputArea.rightOffset = g_outRightOffset;
    outputConfigure->outputArea.upOffset    = g_outUpOffset;
    outputConfigure->outputArea.downOffset  = g_outDownOffset;
    HIAI_ENGINE_LOG("outputArea.leftOffset = %u", outputConfigure->outputArea.leftOffset);
    HIAI_ENGINE_LOG("outputArea.rightOffset = %u", outputConfigure->outputArea.rightOffset);
    HIAI_ENGINE_LOG("outputArea.upOffset = %u", outputConfigure->outputArea.upOffset);
    HIAI_ENGINE_LOG("outputArea.downOffset = %u", outputConfigure->outputArea.downOffset);

    imageConfigure->roiConfigure = roiConfigure.get();

    IDVPPAPI *pVpcIdvppapi = nullptr;
    int32_t ret = CreateDvppApi(pVpcIdvppapi);
    if (ret != 0) {
        HIAI_ENGINE_LOG(HIAI_CREATE_DVPP_ERROR, "create dvpp api fail.");
        HIAI_DVPP_DFree(vpcOutBuffer);
        vpcOutBuffer = nullptr;
        return;
    }
    dvppapi_ctl_msg dvppVpcCtlMsg;
    dvppVpcCtlMsg.in = static_cast<void*>(imageConfigure.get());
    dvppVpcCtlMsg.in_size = sizeof(VpcUserImageConfigure);

    ret = DvppCtl(pVpcIdvppapi, DVPP_CTL_VPC_PROC, &dvppVpcCtlMsg);
    if (ret != 0) {
        HIAI_ENGINE_LOG(HIAI_VPC_CTL_ERROR, "call vpc dvppctl process faild!");
        DestroyDvppApi(pVpcIdvppapi);
        HIAI_DVPP_DFree(jpegdOutData.yuvData);
        HIAI_DVPP_DFree(vpcOutBuffer);
        vpcOutBuffer = nullptr;
        return;
    }

    DestroyDvppApi(pVpcIdvppapi);
    HIAI_ENGINE_LOG("vpc process success.");

    if (jpegdOutData.yuvData != nullptr) {
        HIAI_DVPP_DFree(jpegdOutData.yuvData);
        jpegdOutData.yuvData = nullptr;
    }

    TestJpegdVpcJpegdNewBranch(vpcOutBuffer);
    HIAI_DVPP_DFree(vpcOutBuffer);
    vpcOutBuffer = nullptr;
    HIAI_ENGINE_LOG("jpege process success.");
    return;
}

void TestJpegdVpcJpegdNewBranch(uint8_t* vpcOutBuffer)
{
    // call Jpege process
    sJpegeIn  inData;
    sJpegeOut outData;
    inData.width         = g_outRightOffset - g_outLeftOffset + 1;
    inData.stride        = ALIGN_UP(inData.width, 16);
    inData.height        = g_outDownOffset - g_outUpOffset + 1;
    inData.heightAligned = ALIGN_UP(inData.height, 2);
    inData.format        = JPGENC_FORMAT_NV12;
    inData.level         = 100;
    inData.bufSize = inData.stride * inData.heightAligned * 3 / 2;
    inData.buf = reinterpret_cast<unsigned char*>(vpcOutBuffer);
    unsigned char* tmpAddr = nullptr;

    int32_t ret = DvppGetOutParameter((void*)(&inData), (void*)(&outData), GET_JPEGE_OUT_PARAMETER);
    // 此处获得的jpgSize为估算值，实际数据长度可能要小于这个值
    // 最终jpgSize大小，以调用DvppCtl接口以后，此字段才为真正的编码后的jpgSize大小
    HIAI_ENGINE_LOG("outdata size is %d", outData.jpgSize);
    tmpAddr = (unsigned char*)HIAI_DVPP_DMalloc(outData.jpgSize);
    uint32_t size = outData.jpgSize;

    dvppapi_ctl_msg dvppJpegeCtlMsg;
    dvppJpegeCtlMsg.in = (void *)&inData;
    dvppJpegeCtlMsg.in_size = sizeof(inData);
    dvppJpegeCtlMsg.out = (void *)&outData;
    dvppJpegeCtlMsg.out_size = sizeof(outData);

    IDVPPAPI *pJpegeIDvppApi = nullptr;
    CreateDvppApi(pJpegeIDvppApi);
    do {
        ((sJpegeOut*)(dvppJpegeCtlMsg.out))->jpgData = tmpAddr;
        ((sJpegeOut*)(dvppJpegeCtlMsg.out))->jpgSize = size;
        HIAI_ENGINE_LOG("tmp Addr is %p, size is %u", tmpAddr, size);
        if (pJpegeIDvppApi != nullptr) {
            if (DvppCtl(pJpegeIDvppApi, DVPP_CTL_JPEGE_PROC, &dvppJpegeCtlMsg) != 0) {
                HIAI_ENGINE_LOG(HIAI_JPEGE_CTL_ERROR, "call jpeg encoder fail");
                break;
            }
        }

        FILE* fpOut = fopen(out_file_name, "wb+");
        if (fpOut != nullptr) {
            fwrite(outData.jpgData, 1, outData.jpgSize, fpOut);
            fflush(fpOut);
            fclose(fpOut);
        } else {
            HIAI_ENGINE_LOG(HIAI_JPEGE_CTL_ERROR, "can not save result file");
        }
    } while (0); // for resource pJpegeIDvppApi

    HIAI_ENGINE_LOG(HIAI_OK, "jpeg encode process completed");
    DestroyDvppApi(pJpegeIDvppApi);
    if (tmpAddr != nullptr) { // 注意：释放内存时需要使用tmpAddr，因为outData.jpgData在JPEGE处理后会有地址偏移
        HIAI_DVPP_DFree(tmpAddr);
        tmpAddr = nullptr;
    }
}

void init_test_func_barnch()
{
    // the number after case corresponding the function name
    switch (g_testType) {
        case TEST_PNGD_NUM:
            test_func.push_back(TEST_PNGD); // pngd base func
            break;
        case TEST_PNGD_CUSTOM_MEMORY_NUM:
            test_func.push_back(TEST_PNGD_CUSTOM_MEMORY); // pngd alloc buffer by user func
            break;
        case TEST_JPEGD_VPC_JPEGE_NUM:
            test_func.push_back(TEST_JPEGD_VPC_JPEGE); // jpegd + vpc + jpege(old interface)
            break;
        case TEST_JPEGD_VPC_JPEGE_NEW_NUM: // jpegd + vpc + jpege(new interface)
            test_func.push_back(TEST_JPEGD_VPC_JPEGE_NEW);
            break;
        default:
            break;
    }
}

void init_test_func()
{
    HIAI_ENGINE_LOG("test_type:%d", g_testType);
    // the number after case corresponding the function name
    switch (g_testType) {
        case TEST_VPC_NUM:
            test_func.push_back(TEST_VPC); // new vpc base func
            break;
        case TEST_VPC_1_NUM:
            test_func.push_back(TEST_VPC_1); // new vpc multi func test
            break;
        case TEST_VDEC_NUM:
            test_v_func.push_back(TEST_VDEC); // vdec base func
            break;
        case TEST_VENC_NUM:
            test_func.push_back(TEST_VENC); // venc base func
            break;
        case TEST_JPEGE_NUM:
            test_func.push_back(TEST_JPEGE); // jpege base func
            break;
        case TEST_JPEGE_CUSTOM_MEMORY_NUM:
            test_func.push_back(TEST_JPEGE_CUSTOM_MEMORY); // jpege alloc buffer by user func
            break;
        case TEST_JPEGD_NUM:
            test_func.push_back(TEST_JPEGD); // jpegd base func
            break;
        case TEST_JPEGD_CUSTOM_MEMORY_NUM:
            test_func.push_back(TEST_JPEGD_CUSTOM_MEMORY); // jpegd alloc buffer by user func
            break;
        default:
            init_test_func_barnch();
            break;
    }
}

void get_option_2(int c)
{
    switch (c) {
        case 'C':
            g_transform = atoi(optarg);
            break;
        case 'E':
            g_threadNum = atoi(optarg);
            break;
        default:
            break;
    }
}

void get_option_1(int c)
{
    switch (c) {
        case 'i':
            g_hMin = atoi(optarg);
            break;
        case 'a':
            g_hMax = atoi(optarg);
            break;
        case 'A':
            g_vMax = atoi(optarg);
            break;
        case 'I':
            g_vMin = atoi(optarg);
            break;
        case 'L':
            g_loop = atoi(optarg);
            break;
        case 'd':
            g_outWidth = atoi(optarg);
            break;
        case 'g':
            g_outHigh = atoi(optarg);
            break;
        case 'r':
            g_rank = atoi(optarg);
            break;
        case 'F':
            strcpy_s(in_file_name, sizeof(in_file_name), optarg);
            break;
        case 'O':
            strcpy_s(out_file_name, sizeof(out_file_name), optarg);
            break;
        case 'T':
            g_testType = atoi(optarg);
            break;
        case 'Y':
            g_outUpOffset = atoi(optarg);
            break;
        case 'Z':
            g_outDownOffset = atoi(optarg);
            break;
        default:
            get_option_2(c);
            break;
    }
}

void get_option(int argc, char **argv)
{
    while (1) {
        int option_index = 0;
        struct option long_options[] =
        {
            {"img_width", 1, 0, 'w'},
            {"img_height", 1, 0, 'h'},
            {"in_format", 1, 0, 'f'},
            {"out_format", 1, 0, 'o'},
            {"yuvStoreType", 1, 0, 'b'},
            {"hmin", 1, 0, 'i'},
            {"hmax", 1, 0, 'a'},
            {"vmax", 1, 0, 'A'},
            {"vmin", 1, 0, 'I'},
            {"out_width", 1, 0, 'd'},
            {"out_high", 1, 0, 'g'},
            {"in_image_file", 1, 0, 'F'},
            {"out_image_file", 1, 0, 'O'},
            {"rank", 1, 0, 'r'},
            {"test_type", 1, 0, 'T'},
            {"loop", 1, 0, 'L'},
            {"input_file", 1, 0, 'G'},
            {"transform", 1, 0, 'C'},
            {"thread_num", 1, 0, 'E'},
            {"inWidthStride", 1, 0, 'D'},
            {"inHighStride", 1, 0, 'R'},
            {"outWidthStride", 1, 0, 'U'},
            {"outHighStride", 1, 0, 'P'},
            {"outLeftOffset", 1, 0, 'Q'},
            {"outRightOffset", 1, 0, 'X'},
            {"outUpOffset", 1, 0, 'Y'},
            {"outDownOffset", 1, 0, 'Z'},
        };
        int32_t c = getopt_long(argc, argv, "w:h:f:o:b:i:a:A:I:d:g:F:O:r:T:L:G:C:E:D:R:U:P:Q:X:Y:Z",
            long_options, &option_index);
        if (c == -1) {
            break;
        }
        switch (c) {
            case 'w':
                g_width = atoi(optarg);
                break;
            case 'h':
                g_high = atoi(optarg);
                break;
            case 'f':
                g_format = atoi(optarg);
                break;
            case 'o':
                g_outFormat = atoi(optarg);
                break;
            case 'b':
                g_yuvStoreType = atoi(optarg);
                break;
            case 'D':
                g_inWidthStride = atoi(optarg);
                break;
            case 'R':
                g_inHighStride = atoi(optarg);
                break;
            case 'U':
                g_outWidthStride = atoi(optarg);
                break;
            case 'P':
                g_outHighStride = atoi(optarg);
                break;
            case 'Q':
                g_outLeftOffset = atoi(optarg);
                break;
            case 'X':
                g_outRightOffset = atoi(optarg);
                break;
            default:
                get_option_1(c);
                break;
        }
    }
    return;
}

int main(int argc, char **argv)
{
    if (argc < MIN_PARAM_NUM) {
        HIAI_ENGINE_LOG(HIAI_ERROR, "agrc is too short.");
        return 1;
    }

    get_option(argc, &(*argv));
    init_test_func();
    vector<TEST_V>::iterator iter_begin = test_v_func.begin();
    //for (vector<TEST>::iterator pTest = iter_begin; pTest != test_func.end(); ++pTest) {
    //    (*pTest)();
    //}

    vector<thread> threads;
    for (int i = 0; i < g_threadNum; i++) {
	threads.push_back(std::thread(*iter_begin, i));
    }

    for (auto& t : threads) {
	t.join();
    }

    double all_codec_time = 0;
    double all_convert_time = 0;
    for (int i=0; i<g_threadNum; i++) {
	printf("%d-average codec time: %f\n", i, g_delta_time[i] / 1000000.0f / g_frame_cnt[i]);
        printf("%d-average convert time: %f\n", i, g_delta_convert_time[i] / 1000000.0f / g_frame_cnt[i]);
        all_codec_time += g_delta_time[i] / 1000000.0f / g_frame_cnt[i];
        all_convert_time += g_delta_convert_time[i] / 1000000.0f / g_frame_cnt[i]; 
    }
    printf("thread num:%d\n", g_threadNum);
    printf("ALL average codec time: %f\n", all_codec_time / g_threadNum);
    printf("ALL average convert time: %f\n", all_convert_time / g_threadNum);

    return 0;
}
