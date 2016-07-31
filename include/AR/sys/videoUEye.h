/*
 *	videoUEye.h
 *  ARToolKit5
 *
 *  Video capture module utilising the GStreamer pipeline for AR Toolkit
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

#ifndef AR_VIDEO_UEYE_H
#define AR_VIDEO_UEYE_H


#include <AR/ar.h>
#include <AR/video.h>

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct _AR2VideoParamUEyeT AR2VideoParamUEye;

int                    ar2VideoDispOptionUEye     ( void );
AR2VideoParamUEye     *ar2VideoOpenUEye           ( const char *config_in );
int                    ar2VideoCloseUEye          ( AR2VideoParamUEye *vid );
int                    ar2VideoGetIdUEye          ( AR2VideoParamUEye *vid, ARUint32 *id0, ARUint32 *id1 );
int                    ar2VideoGetSizeUEye        ( AR2VideoParamUEye *vid, int *x,int *y );
AR_PIXEL_FORMAT        ar2VideoGetPixelFormatUEye ( AR2VideoParamUEye *vid );
AR2VideoBufferT       *ar2VideoGetImageUEye       ( AR2VideoParamUEye *vid );
int                    ar2VideoCapStartUEye       ( AR2VideoParamUEye *vid );
int                    ar2VideoCapStopUEye        ( AR2VideoParamUEye *vid );

int                    ar2VideoGetParamiUEye      ( AR2VideoParamUEye *vid, int paramName, int *value );
int                    ar2VideoSetParamiUEye      ( AR2VideoParamUEye *vid, int paramName, int  value );
int                    ar2VideoGetParamdUEye      ( AR2VideoParamUEye *vid, int paramName, double *value );
int                    ar2VideoSetParamdUEye      ( AR2VideoParamUEye *vid, int paramName, double  value );


#ifdef  __cplusplus
}
#endif
#endif // !AR_VIDEO_UEYE_H


