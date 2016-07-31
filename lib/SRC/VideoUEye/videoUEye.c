/*
 *	videoUEye.c
 *  ARToolKit5
 *
 *  Video capture module utilising the IDS UEye SDK for AR Toolkit
 *
 *  This file is part of ARToolKit.
 *
 *  ARToolKit is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  ARToolKit is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with ARToolKit.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  As a special exception, the copyright holders of this library give you
 *  permission to link this library with independent modules to produce an
 *  executable, regardless of the license terms of these independent modules, and to
 *  copy and distribute the resulting executable under terms of your choice,
 *  provided that you also meet, for each linked independent module, the terms and
 *  conditions of the license of that module. An independent module is a module
 *  which is neither derived from nor based on this library. If you modify this
 *  library, you may extend this exception to your version of the library, but you
 *  are not obligated to do so. If you do not wish to do so, delete this exception
 *  statement from your version.
 *
 *  Copyright 2016 Markus Blöchl <markus@blochl.de>
 *
 *  Author(s): Markus Blöchl
 *
 */

#include <AR/video.h>

#include <ueye.h>

/* using memcpy */
#include <string.h>
#include <assert.h>

#define UEYE_RINGBUFFER_SIZE    8

static const char* UEYE_DEFAULT_CFG = "";

struct _AR2VideoParamUEyeT {

	int	width;
    int height;
    AR_PIXEL_FORMAT pixelFormat;

    HCAM cam;
    double framerate;
    double exposure;
    int pixelclk;

    AR2VideoBufferT image;

    char* buf[UEYE_RINGBUFFER_SIZE];
    int   buf_id[UEYE_RINGBUFFER_SIZE];

};


int ar2VideoDispOptionUEye( void )
{
    return 0;
}


AR2VideoParamUEye* ar2VideoOpenUEye( const char *config_in ) {

    const char *config = NULL;
    AR2VideoParamUEye* vid = NULL;

    /* setting up defaults - we fall back to the TV test signal simulator */
    if (!config_in) {
        config = UEYE_DEFAULT_CFG;
    } else if (config_in[0] == '\0') {
        config = UEYE_DEFAULT_CFG;
    } else {
        config = config_in;
    }

    arMalloc( vid, AR2VideoParamUEye, 1 );
    vid->pixelFormat = AR_PIXEL_FORMAT_BGR;
    vid->width       = 800;
    vid->height      = 480;
    vid->framerate   = 17.0;
    vid->exposure    = 100.0;
    vid->pixelclk    = 35;

#define check(stmt) \
    do {    \
        if( IS_SUCCESS != stmt ) { \
            if( !vid->cam ) { \
                ARLOGe("VideoUEye: Could not initialize camera\n"); \
            } else {    \
                int err2 = IS_SUCCESS;  \
                char* msg;  \
                is_GetError(vid->cam, &err2, &msg); \
                if( err2 == IS_SUCCESS ) break; \
                ARLOGe("VideoUEye error: %s\n", msg);  \
            }   \
            free(vid);  \
            return NULL;    \
        }   \
    } while(0)

    const int cam_id = 0; // TODO: cfg
    int n_cams = 0;
    check( is_GetNumberOfCameras(&n_cams) );
    assert( n_cams > cam_id );

    {
        // NOTE: setting up camera
        vid->cam = cam_id;
        check( is_InitCamera(&(vid->cam), 0) );
        //TODO: check( is_GetSensorInfo(vid->cam, &vid->caminfo_) );
        CAMINFO info;
        check( is_GetCameraInfo(vid->cam, &info) );
        // serial_number_ = atoll(info.SerNo);
    }

    {
        // NOTE: set pixelclock 
        int n_clks = 0;
        int clk_list[150];
        memset(clk_list, 0, sizeof(clk_list));
        check( is_PixelClock(vid->cam, IS_PIXELCLOCK_CMD_GET_NUMBER, &n_clks, sizeof(n_clks)) );
        assert( n_clks > 0 );
        check( is_PixelClock(vid->cam, IS_PIXELCLOCK_CMD_GET_LIST, clk_list, n_clks*sizeof(clk_list[0])) );
        assert( vid->pixelclk >= clk_list[0] );
        assert( vid->pixelclk <= clk_list[n_clks-1] );
        for( int i=0; i<n_clks; ++i ) {
            if( vid->pixelclk <= clk_list[i] ) {
                vid->pixelclk = clk_list[i];
                check( is_PixelClock(vid->cam, IS_PIXELCLOCK_CMD_SET, &(vid->pixelclk), sizeof(vid->pixelclk)) );
                break;
            }
        }
    }

    // NOTE: disable autofocus for correct calibration
    check( is_Focus(vid->cam, FOC_CMD_SET_DISABLE_AUTOFOCUS, NULL, 0) );
    // TODO: cfg
    check( is_SetColorMode(vid->cam, IS_CM_BGR8_PACKED) );

    {
        // NOTE: select image format
        // TODO: cfg
        unsigned int n_formats;
        check( is_ImageFormat(vid->cam, IMGFRMT_CMD_GET_NUM_ENTRIES, &n_formats, sizeof(n_formats)) );
        const size_t fmt_list_size = sizeof(IMAGE_FORMAT_LIST) + n_formats*sizeof(IMAGE_FORMAT_INFO);
        IMAGE_FORMAT_LIST* format_list = (IMAGE_FORMAT_LIST*)malloc(fmt_list_size);
        format_list->nSizeOfListEntry = sizeof(IMAGE_FORMAT_INFO);
        format_list->nNumListElements = n_formats;
        check( is_ImageFormat(vid->cam, IMGFRMT_CMD_GET_LIST, format_list, fmt_list_size) );
        for( int i=0; i < n_formats; ++i ) {
            IMAGE_FORMAT_INFO* info = &(format_list->FormatInfo[i]);
            // TODO: ugly, but this is by far the best format
            if( info->nWidth == 800 && (strstr(info->strFormatName, "30 fps")>0)) {
                ARLOGe("Available ImageFormat*: %s", info->strFormatName);
                check( is_ImageFormat(vid->cam, IMGFRMT_CMD_SET_FORMAT, &(info->nFormatID), sizeof(info->nFormatID)) );
            } else {
                ARLOGe("Available ImageFormat : %s", info->strFormatName);
            }
        }
    }

    // TODO: cfg
    check( is_SetFrameRate(vid->cam, vid->framerate, &(vid->framerate)) );

    {
        // NOTE: fixed shutter timing results in far higher available framerate
        double d_zero=0.0;
        double d_dummy=0.0;
        check( is_SetAutoParameter(vid->cam, IS_GET_AUTO_SHUTTER_MAX, &(vid->exposure), NULL) );
        check( is_SetAutoParameter(vid->cam, IS_SET_ENABLE_AUTO_SENSOR_GAIN_SHUTTER, &d_zero, &d_dummy) );
        check( is_Exposure(vid->cam, IS_EXPOSURE_CMD_SET_EXPOSURE, &(vid->exposure), sizeof(vid->exposure)));
    }

    {
        // TODO: would this be useful?
        double d_zero=0.0;
        double d_dummy=0.0;
        is_SetAutoParameter(vid->cam, IS_SET_ENABLE_AUTO_GAIN, &d_zero, &d_dummy);
        // is_SetHWGainFactor(vid->cam, IS_SET_MASTER_GAIN_FACTOR, *gain);
    }

    // NOTE: hardware gamma results in way better lighting without performance impact
    check( is_SetHardwareGamma(vid->cam, IS_SET_HW_GAMMA_ON) );

    // NOTE: disable all zooming
    check( is_SetSubSampling(vid->cam, 0) );
    check( is_SetBinning(vid->cam, 0) );

    // NOTE: trigger not needed
    check( is_SetExternalTrigger(vid->cam, 0) );
    // is_HotPixel(vid->cam, IS_HOTPIXEL_ENABLE_CAMERA_CORRECTION, NULL, 0);
    // TODO: printf("Resolution: %dx%d @%fHz with %dMHz\n", cam.getWidth(), cam.getHeight(), fps, pixclk);

    for( int i=0; i<UEYE_RINGBUFFER_SIZE; ++i ) {
        ARLOGe("PixelSize: %d\n", arVideoUtilGetPixelSize(vid->pixelFormat));
        const int bpp = 24;
        check( is_AllocImageMem(vid->cam, vid->width, vid->height, bpp, &(vid->buf[i]), &(vid->buf_id[i])) );
        check( is_AddToSequence(vid->cam, vid->buf[i], vid->buf_id[i]) );
    }

#undef check

    return vid;
}; //end: ar2VideoOpenUEye()

int ar2VideoCloseUEye(AR2VideoParamUEye *vid) {

    if (!vid) return -1;

    for( int i=0; i<UEYE_RINGBUFFER_SIZE; ++i ) {
        is_FreeImageMem(vid->cam, vid->buf[i], vid->buf_id[i]);
    }
    is_ExitCamera(vid->cam);

	return 0;
}


int ar2VideoGetIdUEye( AR2VideoParamUEye *vid, ARUint32 *id0, ARUint32 *id1 )
{
    if (!vid) return -1;

    *id0 = 0;
    *id1 = 0;

    return -1;
}

int ar2VideoGetSizeUEye(AR2VideoParamUEye *vid, int *x, int *y )
{
    if (!vid) return -1;

    *x = vid->width; // width of your static image
    *y = vid->height; // height of your static image
    return 0;
}

AR_PIXEL_FORMAT ar2VideoGetPixelFormatUEye( AR2VideoParamUEye *vid )
{
    if (!vid) return AR_PIXEL_FORMAT_INVALID;

    return vid->pixelFormat;
}

AR2VideoBufferT* ar2VideoGetImageUEye(AR2VideoParamUEye *vid)
{
    if (!vid) return NULL;

    char* img;

    if (is_WaitEvent(vid->cam, IS_SET_EVENT_FRAME, (int)(2000 / vid->framerate)) == IS_SUCCESS) {
        if (is_GetImageMem(vid->cam, (void**)&img) == IS_SUCCESS) {
            (vid->image).buff = img;
            (vid->image).buffLuma = NULL;
            (vid->image).fillFlag = 1;
        }
    }

    // else { return last image }

    return (&(vid->image));
}

int ar2VideoCapStartUEye(AR2VideoParamUEye *vid)
{
    if (!vid) return -1;
    is_EnableEvent(vid->cam, IS_SET_EVENT_FRAME);

    // There is a weird condition with the uEye SDK 4.30 where
    // this never returns when using IS_WAIT.
    // We are using IS_DONT_WAIT and retry every 0.1s for 2s instead
    for (int i = 0; i < 20; ++i) {
        if ( is_CaptureVideo(vid->cam, IS_DONT_WAIT) == IS_SUCCESS ) {
            return 0;
        }
        usleep(100000); // 100ms
    }
    ARLOGe("VideoUEye: Capture could not be started.");

	return -1;
}

int ar2VideoCapStopUEye(AR2VideoParamUEye *vid)
{
    if (!vid) return -1;

    is_DisableEvent(vid->cam, IS_SET_EVENT_FRAME);
    is_StopLiveVideo(vid->cam, IS_WAIT);

    return 0;
}

int ar2VideoGetParamiUEye( AR2VideoParamUEye *vid, int paramName, int *value )
{
    return -1;
}
int ar2VideoSetParamiUEye( AR2VideoParamUEye *vid, int paramName, int  value )
{
    return -1;
}
int ar2VideoGetParamdUEye( AR2VideoParamUEye *vid, int paramName, double *value )
{
    return -1;
}
int ar2VideoSetParamdUEye( AR2VideoParamUEye *vid, int paramName, double  value )
{
    return -1;
}


