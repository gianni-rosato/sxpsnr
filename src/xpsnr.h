/*
File: xpsnr.h - public declarations for XPSNR measurement filter
Authors: Christian Helmrich and Christian Stoffers, Fraunhofer HHI, Berlin, Germany
        MODIFIED BY EMMIR (LMP88959) to be standalone

License:

The copyright in this software is being made available under this Software Copyright
License. This software may be subject to other third-party and contributor rights,
including patent rights, and no such rights are granted under this license.

Copyright (c) 2019 - 2024 Fraunhofer-Gesellschaft zur Förderung der angewandten
Forschung e.V. (Fraunhofer). All rights reserved.

Redistribution and use of this software in source and binary forms, with or without
modification, are permitted for non-commercial and commercial purposes provided that
the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this list
  of conditions, and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, this
  list of conditions, and the following disclaimer in the documentation and/or other
  materials provided with the distribution.
* Neither the names of the copyright holder nor the names of its contributors may
  be used to endorse or promote products derived from this software without specific
  prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, ARE DISCLAIMED. IN NO EVENT
SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
DAMAGE.

NO PATENTS GRANTED

NO EXPRESS OR IMPLIED LICENSES TO ANY PATENT CLAIMS, INCLUDING WITHOUT LIMITATION
THE PATENTS OF THE COPYRIGHT HOLDER AND CONTRIBUTORS, ARE GRANTED BY THIS SOFTWARE
LICENSE. THE COPYRIGHT HOLDER AND CONTRIBUTORS PROVIDE NO WARRANTY OF PATENT NON-
INFRINGEMENT WITH RESPECT TO THIS SOFTWARE.
*/

#ifndef _XPSNR_H_
#define _XPSNR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* TODO hacks made to just get it to work. */

/* for >8bit video support:
 * change FRAME_ELEM_TYPE to uint16_t,
 * create new YUV/Y4M loading functions for >8bit frames,
 * set XPSNRContext->bpp/depth appropriately in accum().
 * 
 * I think that should be it, but I may have missed something */
#define FRAME_ELEM_TYPE uint8_t
#ifndef bool
#define bool uint8_t
#endif

#define xpsnr_alloc(a, b) malloc((a) * (b))
#define xpsnr_allocz(a) calloc(a,1)
#define xpsnr_free free

/* XPSNR structure definition */

typedef struct XPSNRContext {
    /* required basic variables */
    int bpp; /* unpacked */
    int depth; /* packed */
    int numComps;
    uint64_t numFrames64;
    unsigned frameRate;
    int lineSizes[4];
    int planeHeight[4];
    int planeWidth[4];
    /* XPSNR specific variables */
    double *sseLuma;
    double *weights;
    uint8_t *bufOrg[3];
    uint8_t *bufOrgM1[3];
    uint8_t *bufOrgM2[3];
    uint8_t *bufRec[3];
    uint64_t maxError64;
    double sumWDist[3];
    double sumXPSNR[3];
    bool andIsInf[3];
} XPSNRContext;

typedef struct {
    int width;
    int height;
    int subsamp;
    
    int fps_num;
    int fps_den;
} XPSNR_META;

typedef struct {
    uint8_t *data;
    int len;
    int format;
    int stride;
    int w, h;
} XPSNR_PLANE;

typedef struct {
    XPSNR_PLANE planes[3];
} XPSNR_FRAME;

extern void accum(XPSNRContext *s, XPSNR_FRAME *orig, XPSNR_FRAME *recon, XPSNR_META *meta);
extern double getAvgXPSNR(const double sqrtWSSEData, const double sumXPSNRData,
                          const uint32_t imageWidth, const uint32_t imageHeight,
                          const uint64_t maxError64, const uint64_t numFrames64);

#ifdef __cplusplus
}
#endif
#endif /* _XPSNR_H_ */
