/*=============================================================================
#     FileName: v4l2.c
#         Desc: this program aim to get image from USB camera,
#               used the V4L2 interface.
#       Author: Licaibiao
#      Version: 
#   LastChange: 2016-12-10 
#      History:
=============================================================================*/
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <getopt.h>
#include <linux/types.h>
#include <linux/videodev2.h>
#include <malloc.h>
#include <math.h>
#include <string.h>
#include <sys/mman.h>
#include <errno.h>
#include <assert.h>
 
#define FILE_VIDEO  "/dev/video0"
#define JPG "./image%d"
 
typedef struct{
    void *start;
	int length;
}BUFTYPE;
 
BUFTYPE *usr_buf;
static unsigned int n_buffer = 0;
uint8_t frameNum=0;
char* path;
 
/*set video capture ways(mmap)*/
int init_mmap(int fd)
{
	/*to request frame cache, contain requested counts*/
	struct v4l2_requestbuffers reqbufs;
 
	memset(&reqbufs, 0, sizeof(reqbufs));
	reqbufs.count = 1; 	 							/*the number of buffer*/
	reqbufs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;    
	reqbufs.memory = V4L2_MEMORY_MMAP;				
 
	if(-1 == ioctl(fd,VIDIOC_REQBUFS,&reqbufs))
	{
		perror("Fail to ioctl 'VIDIOC_REQBUFS'");
		exit(EXIT_FAILURE);
	}
	
	n_buffer = reqbufs.count;
	printf("n_buffer = %d\n", n_buffer);
	//usr_buf = calloc(reqbufs.count, sizeof(usr_buf));
	usr_buf = calloc(reqbufs.count, sizeof(BUFTYPE));
	if(usr_buf == NULL)
	{
		printf("Out of memory\n");
		exit(-1);
	}
 
	/*map kernel cache to user process*/
	for(n_buffer = 0; n_buffer < reqbufs.count; ++n_buffer)
	{
		//stand for a frame
		struct v4l2_buffer buf;
		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = n_buffer;
		
		/*check the information of the kernel cache requested*/
		if(-1 == ioctl(fd,VIDIOC_QUERYBUF,&buf))
		{
			perror("Fail to ioctl : VIDIOC_QUERYBUF");
			exit(EXIT_FAILURE);
		}
 
		usr_buf[n_buffer].length = buf.length;
		usr_buf[n_buffer].start = (char *)mmap(NULL,buf.length,PROT_READ | PROT_WRITE,MAP_SHARED, fd,buf.m.offset);
 
		if(MAP_FAILED == usr_buf[n_buffer].start)
		{
			perror("Fail to mmap");
			exit(EXIT_FAILURE);
		}
 
	}
    return 0; 
}
 
int open_camera(void)
{
	int fd;
	struct v4l2_input inp;
 
	fd = open(FILE_VIDEO, O_RDWR | O_NONBLOCK,0);
	if(fd < 0)
	{	
		fprintf(stderr, "%s open err \n", FILE_VIDEO);
		exit(EXIT_FAILURE);
	};
 
	inp.index = 0;
	if (-1 == ioctl (fd, VIDIOC_S_INPUT, &inp))
	{
		fprintf(stderr, "VIDIOC_S_INPUT \n");
	}
 
	return fd;
}
 
int init_camera(int fd)
{
	struct v4l2_capability 	cap;	/* decive fuction, such as video input */
	struct v4l2_format 		tv_fmt; /* frame format */  
	struct v4l2_fmtdesc 	fmtdesc;  	/* detail control value */
	struct v4l2_control 	ctrl;
	int ret;
	
			/*show all the support format*/
	memset(&fmtdesc, 0, sizeof(fmtdesc));
	fmtdesc.index = 0 ;                 /* the number to check */
	fmtdesc.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
 
	/* check video decive driver capability */
	if(ret=ioctl(fd, VIDIOC_QUERYCAP, &cap)<0)
	{
		fprintf(stderr, "fail to ioctl VIDEO_QUERYCAP \n");
		exit(EXIT_FAILURE);
	}
	
	/*judge wherher or not to be a video-get device*/
	if(!(cap.capabilities & V4L2_BUF_TYPE_VIDEO_CAPTURE))
	{
		fprintf(stderr, "The Current device is not a video capture device \n");
		exit(EXIT_FAILURE);
	}
 
	/*judge whether or not to supply the form of video stream*/
	if(!(cap.capabilities & V4L2_CAP_STREAMING))
	{
		printf("The Current device does not support streaming i/o\n");
		exit(EXIT_FAILURE);
	}
	
	printf("\ncamera driver name is : %s\n",cap.driver);
	printf("camera device name is : %s\n",cap.card);
	printf("camera bus information: %s\n",cap.bus_info);
 
	/*display the format device support*/
	printf("\n");
	while(ioctl(fd,VIDIOC_ENUM_FMT,&fmtdesc)!=-1)
	{	
		printf("support device %d.%s\n",fmtdesc.index+1,fmtdesc.description);
		fmtdesc.index++;
	}
	printf("\n");
 
	/*set the form of camera capture data*/
	tv_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;      /*v4l2_buf_typea,camera must use V4L2_BUF_TYPE_VIDEO_CAPTURE*/
	tv_fmt.fmt.pix.width = 640;
	tv_fmt.fmt.pix.height = 480;
	//tv_fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;	/*V4L2_PIX_FMT_YYUV*/
	tv_fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;	/*V4L2_PIX_FMT_YYUV*/
	tv_fmt.fmt.pix.field = V4L2_FIELD_NONE;   		/*V4L2_FIELD_NONE*/
	if (ioctl(fd, VIDIOC_S_FMT, &tv_fmt)< 0) 
	{
		fprintf(stderr,"VIDIOC_S_FMT set err\n");
		exit(-1);
		close(fd);
	}
 
	init_mmap(fd);
    return 0;
}
 
int start_capture(int fd)
{
	unsigned int i;
	enum v4l2_buf_type type;
	
	/*place the kernel cache to a queue*/
	for(i = 0; i < n_buffer; i++)
	{
		struct v4l2_buffer buf;
		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;
 
		if(-1 == ioctl(fd, VIDIOC_QBUF, &buf))
		{
			perror("Fail to ioctl 'VIDIOC_QBUF'");
			exit(EXIT_FAILURE);
		}
	}
 
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if(-1 == ioctl(fd, VIDIOC_STREAMON, &type))
	{
		printf("i=%d.\n", i);
		perror("VIDIOC_STREAMON");
		close(fd);
		exit(EXIT_FAILURE);
	}
 
	return 0;
}
 
 
int process_image(char* outputFile, void *addr, int length);
int init_decoder(void);
void destroy_decoder(void);
 
int read_frame(int fd)
{
	struct v4l2_buffer buf;
	unsigned int i;
	memset(&buf, 0, sizeof(buf));
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	//put cache from queue
	if(-1 == ioctl(fd, VIDIOC_DQBUF,&buf))
	{
		perror("Fail to ioctl 'VIDIOC_DQBUF'");
		exit(EXIT_FAILURE);
	}
	assert(buf.index < n_buffer);
 
	//read process space's data to a file
	//process_image(usr_buf[buf.index].start, usr_buf[buf.index].length);
	process_image(path, usr_buf[buf.index].start, buf.bytesused);
	if(-1 == ioctl(fd, VIDIOC_QBUF,&buf))
	{
		perror("Fail to ioctl 'VIDIOC_QBUF'");
		exit(EXIT_FAILURE);
	}
	return 1;
}
 
 
int mainloop(int fd)
{
	while(frameNum-- > 0)
	{
		for(;;)
		{
			fd_set fds;
			struct timeval tv;
			int r;
 
			FD_ZERO(&fds);
			FD_SET(fd,&fds);
 
			/*Timeout*/
			tv.tv_sec = 2;
			tv.tv_usec = 0;
			r = select(fd + 1,&fds,NULL,NULL,&tv);
			
			if(-1 == r)
			{
				 if(EINTR == errno)
					continue;
				perror("Fail to select");
				exit(EXIT_FAILURE);
			}
			if(0 == r)
			{
				fprintf(stderr,"select Timeout\n");
				exit(-1);
			}
 
			if(read_frame(fd))
			break;
		}
	}
	return 0;
}
 
void stop_capture(int fd)
{
	enum v4l2_buf_type type;
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if(-1 == ioctl(fd,VIDIOC_STREAMOFF,&type))
	{
		perror("Fail to ioctl 'VIDIOC_STREAMOFF'");
		exit(EXIT_FAILURE);
	}
}
 
void close_camera_device(int fd)
{
	unsigned int i;
	for(i = 0;i < n_buffer; i++)
	{
		if(-1 == munmap(usr_buf[i].start,usr_buf[i].length))
		{
			exit(-1);
		}
	}
 
	free(usr_buf);
 
	if(-1 == close(fd))
	{
		perror("Fail to close fd");
		exit(EXIT_FAILURE);
	}
}

void parseArgument(int argc, char **argv)
{
    const struct option long_opts[] = {
        {"help", no_argument, NULL, 0 },
        {"bitrate", required_argument, NULL, 1 },
        {"compare", required_argument, NULL, 2 },
        {"qpmin", required_argument, NULL, 3 },
        {"qpmax", required_argument, NULL, 4 },
        {"avc", no_argument, NULL, 5 },
        {"sar", no_argument, NULL, 6 },
        {"fixqp", no_argument, NULL, 7 },
        {"iqp", required_argument, NULL, 8 },
        {"pqp", required_argument, NULL, 9 },
        {"sliceNum", required_argument, NULL, 10 },
        {"intraPeriod", required_argument, NULL, 11 },
        {"intraRefresh", no_argument, NULL, 12 },
        {"blockNum", required_argument, NULL, 13 },
        {"fast", no_argument, NULL, 14 },
        {"super", no_argument, NULL, 15 },
        {"vbv", required_argument, NULL, 16 },
        {"hh", no_argument, NULL, 69},
        {NULL, no_argument, NULL, 0 }
    };

    path = malloc(128*sizeof(char));
    memset(path, 0, strlen(path));
    strncpy(path, "./jgl.yuv", 7);

    int long_index, i ;
    int c;
    while (1){
	c = getopt_long_only(argc, argv, "b:s:i:d:c:n:f:o:", long_opts, &long_index);

	if (c == -1)
		break;

        switch (c) {
        case 69:
        case 0:
            //PrintDemoUsage();
            exit(0);
        case  'b':
            break;
        case  3:
            break;
        case  4:
            break;
        case  5:
            break;
        case  6:
            break;
        case  7:
            break;
        case  8:
            break;
        case  9:
            break;
        case  10:
            break;
        case  11:
            break;
        case  12:
            break;
        case  13:
            break;
        case  14:
            break;
        case  15:
            break;
        case  16:
            break;
        case  2:
        case 's':
            break;
        case 'd':
            break;
        case 'c':
            break;
        case 'n':
            frameNum = atoi(optarg);
            break;
        case 'f':
            break;
        case 'i':
            break;
        case 'o':
            memset(path, 0, strlen(path));
            strncpy(path, optarg, strlen(optarg));
            break;

	default:
	    break;
	}
    }
}

int main(int argc, char **argv)
{
    parseArgument(argc, argv);
	int fd;

	fd = open_camera();
	init_camera(fd);
    init_decoder();
	start_capture(fd);
	mainloop(fd);
	stop_capture(fd);
    destroy_decoder();
	close_camera_device(fd);
    return 0;
}

