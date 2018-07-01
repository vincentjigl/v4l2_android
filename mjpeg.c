/*
 * Copyright (c) 2008-2016 Allwinner Technology Co. Ltd.
 * All rights reserved.
 *
 * File : demojpeg.c
 * Description : demojpeg
 * History :
 *
 */

#include <stdio.h>

#include <cdx_log.h>
#include <vdecoder.h>
#include "memoryAdapter.h"
#include "Libve_Decoder2.h"
#include <errno.h>
#include <sys/time.h>

typedef struct VideoFrame
{
    // Intentional public access modifier:
    uint32_t mWidth;
    uint32_t mHeight;
    uint32_t mDisplayWidth;
    uint32_t mDisplayHeight;
    uint32_t mSize;            // Number of bytes in mData
    uint8_t* mData;            // Actual binary data
    int32_t  mRotationAngle;   // rotation angle, clockwise
}VideoFrame;

uint8_t *outPtr = NULL; 
VideoDecoder *pVideo;
VideoStreamInfo videoInfo;
VideoStreamDataInfo videoDataInfo;
VConfig videoConfig;
VideoPicture *videoPicture = NULL;
struct ScMemOpsS* memops = NULL;
extern uint8_t frameNum;

static int dumpData(char* outputFile, uint8_t *data, int len)
{
    FILE *fp;
    printf("output file %s", outputFile);
    fp = fopen(outputFile, "a");

    if(fp != NULL)
    {
        if(frameNum%2)
            printf("\rdump data %d ...", len);
        else 
            printf("\rdump data %d ......", len);
        fwrite(data, 1, len, fp);
        fclose(fp);
    }
    else
    {
        printf("saving picture open file error, errno(%d)\n", errno);
        return -1;
    }

    return 0;
}

int init_decoder(void)
{
    int ret;

    memops = MemAdapterGetOpsS();
    if(memops == NULL)
    {
        return -1;
    }
    CdcMemOpen(memops);

    memset(&videoConfig, 0x00, sizeof(VConfig));
    videoConfig.bDisable3D = 0;
    videoConfig.bDispErrorFrame = 0;
    videoConfig.bNoBFrames = 0;
    videoConfig.bRotationEn = 0;
    videoConfig.bScaleDownEn = 0;
    videoConfig.nHorizonScaleDownRatio = 0;
    videoConfig.nVerticalScaleDownRatio = 0;
    videoConfig.eOutputPixelFormat =PIXEL_FORMAT_YV12;
    videoConfig.nDeInterlaceHoldingFrameBufferNum = 0;
    videoConfig.nDisplayHoldingFrameBufferNum = 0;
    videoConfig.nRotateHoldingFrameBufferNum = 0;
    videoConfig.nDecodeSmoothFrameBufferNum = 0;
    videoConfig.nVbvBufferSize = 2*1024*1024;
    videoConfig.bThumbnailMode = 1;
    videoConfig.memops = memops;

    memset(&videoInfo, 0x00, sizeof(VideoStreamInfo));
    videoInfo.eCodecFormat = VIDEO_CODEC_FORMAT_MJPEG;
    videoInfo.nWidth = 640;
    videoInfo.nHeight = 480;
    videoInfo.nFrameRate = 30;
    videoInfo.nFrameDuration = 1000*1000/30;
    videoInfo.nAspectRatio = 1000;
    videoInfo.bIs3DStream = 0;
    videoInfo.nCodecSpecificDataLen = 0;
    videoInfo.pCodecSpecificData = NULL;

    memset(&videoDataInfo, 0, sizeof(VideoStreamDataInfo));
    outPtr = malloc(videoInfo.nWidth * videoInfo.nHeight *3/ 2);

    Libve_init2(&pVideo, &videoInfo, &videoConfig);
    if(!pVideo)
    {
        printf("create video decoder failed\n");
        return 0;
    }
    printf("Initialize video decoder ok\n");

    return 0;
}

void destroy_decoder(void)
{
    Libve_exit2(&pVideo);
    
}

int process_image(char* outputFile, void *addr, int length)
{
    memset(outPtr, 0, videoInfo.nWidth * videoInfo.nHeight *3/ 2);
    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv, &tz);

    videoDataInfo.nLength = length;
    videoDataInfo.nPts = (int64_t)(tv.tv_sec*1000000+tv.tv_usec/1000);
    Libve_dec2(&pVideo, addr, (void*)outPtr, &videoInfo, &videoDataInfo, &videoConfig);
    dumpData(outputFile, outPtr, videoInfo.nWidth * videoInfo.nHeight *3/ 2);

    return 0;
}

