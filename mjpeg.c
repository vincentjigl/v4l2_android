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
#include <errno.h>

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

VConfig vConfig;
VideoDecoder *pVideo;
VideoPicture *videoPicture = NULL;
struct ScMemOpsS* memops = NULL;


static int dumpData(char *path, uint8_t *data, int len)
{
    FILE *fp;
    fp = fopen(path, "a+");

    if(fp != NULL)
    {
        printf("dump data '%d'", len);
        fwrite(data, 1, len, fp);
        fclose(fp);
    }
    else
    {
        printf("saving picture open file error, errno(%d)", errno);
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

    memset(&vConfig, 0x00, sizeof(VConfig));
    vConfig.bDisable3D = 0;
    vConfig.bDispErrorFrame = 0;
    vConfig.bNoBFrames = 0;
    vConfig.bRotationEn = 0;
    vConfig.bScaleDownEn = 0;
    vConfig.nHorizonScaleDownRatio = 0;
    vConfig.nVerticalScaleDownRatio = 0;
    vConfig.eOutputPixelFormat =PIXEL_FORMAT_YUV_MB32_420;
    vConfig.nDeInterlaceHoldingFrameBufferNum = 0;
    vConfig.nDisplayHoldingFrameBufferNum = 0;
    vConfig.nRotateHoldingFrameBufferNum = 0;
    vConfig.nDecodeSmoothFrameBufferNum = 0;
    vConfig.nVbvBufferSize = 2*1024*1024;
    vConfig.bThumbnailMode = 1;
    vConfig.memops = memops;
    VideoStreamInfo videoInfo;
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

    pVideo = CreateVideoDecoder();
    if(!pVideo)
    {
        printf("create video decoder failed\n");
        return 0;
    }
    printf("create video decoder ok\n");

    if ((InitializeVideoDecoder(pVideo, &videoInfo, &vConfig)) != 0)
    {
        printf("open dev failed,  decode error !\n");
        return 0;
    }
    printf("Initialize video decoder ok\n");

    return 0;
}

void destroy_decoder(void)
{
    
    CdcMemClose(memops);
}

int process_image(void *addr, int length)
{
    int ret;
    char *buf, *ringBuf;
    int buflen, ringBufLen;
    char*jpegData=addr; 
    int dataLen = length;

    if(RequestVideoStreamBuffer(pVideo,
                                dataLen,
                                (char**)&buf,
                                &buflen,
                                (char**)&ringBuf,
                                &ringBufLen,
                                0))
    {
        printf("Request Video Stream Buffer failed\n");
        return 0;
    }
    printf("Request Video Stream Buffer ok\n");

    if(buflen + ringBufLen < dataLen)
    {
        printf("#####Error: request buffer failed, buffer is not enough\n");
        return 0;
    }

    printf("goto to copy Video Stream Data ok!\n");
    // copy stream to video decoder SBM
    if(buflen >= dataLen)
    {
        memcpy(buf,jpegData,dataLen);
    }
    else
    {
        memcpy(buf,jpegData,buflen);
        memcpy(ringBuf,jpegData+buflen,dataLen-buflen);
    }
    printf("Copy Video Stream Data ok!\n");

    VideoStreamDataInfo DataInfo;
    memset(&DataInfo, 0, sizeof(DataInfo));
    DataInfo.pData = buf;
    DataInfo.nLength = dataLen;
    DataInfo.bIsFirstPart = 1;
    DataInfo.bIsLastPart = 1;

    if (SubmitVideoStreamData(pVideo, &DataInfo, 0))
    {
        printf("#####Error: Submit Video Stream Data failed!\n");
        return 0;
    }
    printf("Submit Video Stream Data ok!\n");

    // step : decode stream now
    int endofstream = 0;
    int dropBFrameifdelay = 0;
    int64_t currenttimeus = 0;
    int decodekeyframeonly = 1;

   ret = DecodeVideoStream(pVideo, endofstream, decodekeyframeonly,
                        dropBFrameifdelay, currenttimeus);
    printf("decoder ret is %d \n",ret);
    switch (ret)
    {
        case VDECODE_RESULT_KEYFRAME_DECODED:
        case VDECODE_RESULT_FRAME_DECODED:
        case VDECODE_RESULT_NO_FRAME_BUFFER:
        {
            ret = ValidPictureNum(pVideo, 0);
            if (ret>= 0)
            {
                videoPicture = RequestPicture(pVideo, 0);
                if (videoPicture == NULL){
                    printf("decoder fail \n");
                    return 0;
                }
                printf("decoder one pic...\n");
                printf("pic nWidth is %d,nHeight is %d\n",videoPicture->nWidth,videoPicture->nHeight);

                VideoFrame jpegData;
                jpegData.mWidth = videoPicture->nWidth;
                jpegData.mHeight = videoPicture->nHeight;
                jpegData.mSize = jpegData.mWidth*jpegData.mWidth*3/2;
                jpegData.mData = (unsigned char*)malloc(jpegData.mSize);
                if(jpegData.mData == NULL)
                {
                    return -1;
                }

                //char path[1024] = "./pic.rgb";
                char path[1024] = "./jgl.yuv";
                dumpData(path, (uint8_t *)jpegData.mData, jpegData.mWidth * jpegData.mHeight *3/ 2);
                sync();
            }
            else
            {
               printf("no ValidPictureNum ret is %d",ret);
            }

            break;
        }

        case VDECODE_RESULT_OK:
        case VDECODE_RESULT_CONTINUE:
        case VDECODE_RESULT_NO_BITSTREAM:
        case VDECODE_RESULT_RESOLUTION_CHANGE:
        case VDECODE_RESULT_UNSUPPORTED:
        default:
            printf("video decode Error: %d!\n", ret);
            return 0;
    }

    return 0;
}

