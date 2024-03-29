#ifndef __CAMERA_H
#define __CAMERA_H

#include "common.h"
#include "jpeglib.h"


#ifdef __cplusplus
extern "C" {
#endif

#define SQLite2video   "/tmp/SQLite2video"
#define video2SQLite   "/tmp/video2SQLite"


extern int CAMERA_W;
extern int CAMERA_H;

extern int R[256][256];
extern int G[256][256][256];
extern int B[256][256];

extern char formats[5][16];
extern struct v4l2_fmtdesc fmtdesc;
extern struct v4l2_format  fmt;
extern struct v4l2_capability cap;

bool get_caminfo(int camfd);
bool get_camfmt(int camfd);
bool get_camcap(int camfd);

void set_camfmt(int camfd);

void *convert(void *arg);
void yuv2jpg(uint8_t *yuv);


#ifdef __cplusplus
}
#endif

#endif
