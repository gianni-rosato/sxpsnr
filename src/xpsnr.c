/*
File: xpsnr.c - code definitions for XPSNR measurement
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

/*
 * @file
 * Calculate the extended perceptually weighted PSNR (XPSNR) between two input videos.
 */

#include "xpsnr.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

/* required macro definitions */

#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif
#define OFFSET(x) offsetof(XPSNRContext, x)
#define XPSNR_GAMMA 2

/* XPSNR function definitions */
static uint64_t
highds(const int xAct, const int yAct, const int wAct, const int hAct, const FRAME_ELEM_TYPE *o, const int O)
{
    uint64_t saAct = 0;
    int x, y;
    for (y = yAct; y < hAct; y += 2) {
        for (x = xAct; x < wAct; x += 2) {
            const int f = 12 * ((int)o[ y   *O + x  ] + (int)o[ y   *O + x+1] + (int)o[(y+1)*O + x  ] + (int)o[(y+1)*O + x+1])
                   - 3 * ((int)o[(y-1)*O + x  ] + (int)o[(y-1)*O + x+1] + (int)o[(y+2)*O + x  ] + (int)o[(y+2)*O + x+1])
                   - 3 * ((int)o[ y   *O + x-1] + (int)o[ y   *O + x+2] + (int)o[(y+1)*O + x-1] + (int)o[(y+1)*O + x+2])
                   - 2 * ((int)o[(y-1)*O + x-1] + (int)o[(y-1)*O + x+2] + (int)o[(y+2)*O + x-1] + (int)o[(y+2)*O + x+2])
                       - ((int)o[(y-2)*O + x-1] + (int)o[(y-2)*O + x  ] + (int)o[(y-2)*O + x+1] + (int)o[(y-2)*O + x+2]
                        + (int)o[(y+3)*O + x-1] + (int)o[(y+3)*O + x  ] + (int)o[(y+3)*O + x+1] + (int)o[(y+3)*O + x+2]
                        + (int)o[(y-1)*O + x-2] + (int)o[ y   *O + x-2] + (int)o[(y+1)*O + x-2] + (int)o[(y+2)*O + x-2]
                        + (int)o[(y-1)*O + x+3] + (int)o[ y   *O + x+3] + (int)o[(y+1)*O + x+3] + (int)o[(y+2)*O + x+3]);
            saAct += (uint64_t) abs(f);
        }
    }
    return saAct;
}

static uint64_t
diff1st(const uint32_t wAct, const uint32_t hAct, const FRAME_ELEM_TYPE *o, FRAME_ELEM_TYPE *oM1, const int O)
{
    uint64_t taAct = 0;
    uint32_t x, y;
    
    for (y = 0; y < hAct; y += 2) {
        for (x = 0; x < wAct; x += 2) {
            const int t = (int)o  [y*O + x] +
                          (int)o  [y*O + x+1] +
                          (int)o  [(y+1)*O + x] +
                          (int)o  [(y+1)*O + x+1] -
                          ((int)oM1[y*O + x] +
                           (int)oM1[y*O + x+1] +
                           (int)oM1[(y+1)*O + x] +
                           (int)oM1[(y+1)*O + x+1]);
            taAct += (uint64_t) abs(t);
            oM1[y*O + x  ] = o  [y*O + x  ];  oM1[(y+1)*O + x  ] = o  [(y+1)*O + x  ];
            oM1[y*O + x+1] = o  [y*O + x+1];  oM1[(y+1)*O + x+1] = o  [(y+1)*O + x+1];
        }
    }
    return (taAct * XPSNR_GAMMA);
}

static uint64_t
diff2nd(const uint32_t wAct, const uint32_t hAct, const FRAME_ELEM_TYPE *o, FRAME_ELEM_TYPE *oM1, FRAME_ELEM_TYPE *oM2, const int O)
{
  uint64_t taAct = 0;
  uint32_t x, y;
    
    for (y = 0; y < hAct; y += 2) {
        for (x = 0; x < wAct; x += 2) {
            const int t = (int)o  [y*O + x] + (int)o  [y*O + x+1] + (int)o  [(y+1)*O + x] + (int)o  [(y+1)*O + x+1]
                           - 2 * ((int)oM1[y*O + x] + (int)oM1[y*O + x+1] + (int)oM1[(y+1)*O + x] + (int)oM1[(y+1)*O + x+1])
                                + (int)oM2[y*O + x] + (int)oM2[y*O + x+1] + (int)oM2[(y+1)*O + x] + (int)oM2[(y+1)*O + x+1];
            taAct += (uint64_t) abs(t);
            oM2[y*O + x  ] = oM1[y*O + x  ];  oM2[(y+1)*O + x  ] = oM1[(y+1)*O + x  ];
            oM2[y*O + x+1] = oM1[y*O + x+1];  oM2[(y+1)*O + x+1] = oM1[(y+1)*O + x+1];
            oM1[y*O + x  ] = o  [y*O + x  ];  oM1[(y+1)*O + x  ] = o  [(y+1)*O + x  ];
            oM1[y*O + x+1] = o  [y*O + x+1];  oM1[(y+1)*O + x+1] = o  [(y+1)*O + x+1];
        }
    }
  return (taAct * XPSNR_GAMMA);
}

static uint64_t
sseLine(const uint8_t *blkOrg8, const uint8_t *blkRec8, int blockWidth)
{
    const FRAME_ELEM_TYPE *blkOrg = (const FRAME_ELEM_TYPE*) blkOrg8;
    const FRAME_ELEM_TYPE *blkRec = (const FRAME_ELEM_TYPE*) blkRec8;
    uint64_t lSSE = 0; /* data for 1 pixel line */
    int x;
    
    for (x = 0; x < blockWidth; x++) {
        const int64_t error = (int64_t) blkOrg[x] - (int64_t) blkRec[x];
        
        lSSE += error * error;
    }
    
    /* sum of squared errors for the pixel line */
    return lSSE;
}

static uint64_t
calcSquaredError(const FRAME_ELEM_TYPE *blkOrg,     const uint32_t strideOrg,
                 const FRAME_ELEM_TYPE *blkRec,     const uint32_t strideRec,
                 const uint32_t blockWidth, const uint32_t blockHeight)
{
    uint64_t uSSE = 0; /* sum of squared errors */
    uint32_t y;
    for (y = 0; y < blockHeight; y++) {
        uSSE += sseLine((const uint8_t*) blkOrg, (const uint8_t*) blkRec,
                (int) blockWidth);
        blkOrg += strideOrg;
        blkRec += strideRec;
    }
    
    /* return nonweighted sum of squared errors */
    return uSSE;
}

static double
calcSquaredErrorAndWeight(XPSNRContext const *s,
                                                const FRAME_ELEM_TYPE *picOrg,     const uint32_t strideOrg,
                                                FRAME_ELEM_TYPE       *picOrgM1,   FRAME_ELEM_TYPE       *picOrgM2,
                                                const FRAME_ELEM_TYPE *picRec,     const uint32_t strideRec,
                                                const uint32_t offsetX,    const uint32_t offsetY,
                                                const uint32_t blockWidth, const uint32_t blockHeight,
                                                const uint32_t bitDepth,   const uint32_t intFrameRate, double *msAct)
{
    const int      O = (int) strideOrg;
    const int      R = (int) strideRec;
    const FRAME_ELEM_TYPE *o = picOrg   + offsetY*O + offsetX;
    FRAME_ELEM_TYPE     *oM1 = picOrgM1 + offsetY*O + offsetX;
    FRAME_ELEM_TYPE     *oM2 = picOrgM2 + offsetY*O + offsetX;
    const FRAME_ELEM_TYPE *r = picRec   + offsetY*R + offsetX;
    const int   bVal = (s->planeWidth[0] * s->planeHeight[0] > 2048 * 1152 ? 2 : 1); /* threshold is a bit more than HD resolution */
    const int   xAct = (offsetX > 0 ? 0 : bVal);
    const int   yAct = (offsetY > 0 ? 0 : bVal);
    const int   wAct = (offsetX + blockWidth  < (uint32_t) s->planeWidth [0] ? (int) blockWidth  : (int) blockWidth  - bVal);
    const int   hAct = (offsetY + blockHeight < (uint32_t) s->planeHeight[0] ? (int) blockHeight : (int) blockHeight - bVal);
    
    const double sse = (double) calcSquaredError (o, strideOrg,
            r, strideRec,
            blockWidth, blockHeight);
    uint64_t saAct = 0; /* spatial abs. activity */
    uint64_t taAct = 0; /* temporal abs. activity */
    
    if (wAct <= xAct || hAct <= yAct) /* too tiny */
    {
        return sse;
    }
    
    if (bVal > 1) /* highpass with downsampling */
    {
        saAct = highds(xAct, yAct, wAct, hAct, o, O);
    } else /* <=HD, highpass without downsampling */
    {
        int x, y;
        for (y = yAct; y < hAct; y++) {
            for (x = xAct; x < wAct; x++) {
                const int f = 12 * (int)o[y*O + x] - 2 * ((int)o[y*O + x-1] + (int)o[y*O + x+1] + (int)o[(y-1)*O + x] + (int)o[(y+1)*O + x])
                                        - ((int)o[(y-1)*O + x-1] + (int)o[(y-1)*O + x+1] + (int)o[(y+1)*O + x-1] + (int)o[(y+1)*O + x+1]);
                saAct += (uint64_t) abs(f);
            }
        }
    }
    
    /* calculate weight (mean squared activity) */
    *msAct = (double) saAct / ((double)(wAct - xAct) * (double)(hAct - yAct));
    
    if (bVal > 1) /* highpass with downsampling */
    {
        if (intFrameRate <= 32) /* 1st-order diff */
        {
            taAct = diff1st (blockWidth, blockHeight, o, oM1, O);
        }
        else  /* 2nd-order diff (diff of 2 diffs) */
        {
            taAct = diff2nd(blockWidth, blockHeight, o, oM1, oM2, O);
        }
    }
    else /* <=HD, highpass without downsampling */
    {
        uint32_t x, y;
        if (intFrameRate <= 32) /* 1st-order diff */
        {
            for (y = 0; y < blockHeight; y++) {
                for (x = 0; x < blockWidth; x++) {
                    const int t = (int) o[y * O + x] - (int) oM1[y * O + x];
                    
                    taAct += XPSNR_GAMMA * (uint64_t) abs(t);
                    oM1[y * O + x] = o[y * O + x];
                }
            }
        } else /* 2nd-order diff (diff of 2 diffs) */
        {
            for (y = 0; y < blockHeight; y++) {
                for (x = 0; x < blockWidth; x++) {
                    const int t = (int) o[y * O + x] - 2 * (int) oM1[y * O + x]
                            + (int) oM2[y * O + x];
                    
                    taAct += XPSNR_GAMMA * (uint64_t) abs(t);
                    oM2[y * O + x] = oM1[y * O + x];
                    oM1[y * O + x] = o[y * O + x];
                }
            }
        }
    }
    
    /* weight += mean squared temporal activity */
    *msAct += (double) taAct / ((double) blockWidth * (double) blockHeight);
    
    /* lower limit, accounts for high-pass gain */
    if (*msAct < (double)(1 << (bitDepth - 6))) *msAct = (double)(1 << (bitDepth - 6));
    
    *msAct *= *msAct; /* because SSE is squared */
    
    /* return nonweighted sum of squared errors */
    return sse;
}

extern double
getAvgXPSNR(const double sqrtWSSEData, const double sumXPSNRData,
                                  const uint32_t imageWidth, const uint32_t imageHeight,
                                  const uint64_t maxError64, const uint64_t numFrames64)
{
  if (numFrames64 == 0) return INFINITY;

  if (sqrtWSSEData >= (double) numFrames64) /* sq.-mean-root dist averaging */
  {
    const double meanDist = sqrtWSSEData / (double) numFrames64;
    const uint64_t  num64 = (uint64_t) imageWidth * (uint64_t) imageHeight * maxError64;

    return 10.0 * log10 ((double) num64 / ((double) meanDist * (double) meanDist));
  }

  return sumXPSNRData / (double) numFrames64; /* older log-domain averaging */
}

static int
getWSSE(XPSNRContext *s, FRAME_ELEM_TYPE **org, FRAME_ELEM_TYPE **orgM1, FRAME_ELEM_TYPE **orgM2, FRAME_ELEM_TYPE **rec, uint64_t* const wsse64)
{
  const uint32_t      W = s->planeWidth [0];  /* luma image width in pixels */
  const uint32_t      H = s->planeHeight[0]; /* luma image height in pixels */
  const double        R = (double)(W * H) / (3840.0 * 2160.0); /* UHD ratio */
  const uint32_t      B = MAX (0, 4 * (int32_t)(32.0 * sqrt (R) + 0.5)); /* block size, integer multiple of 4 for SIMD */
  const uint32_t   WBlk = (W + B - 1) / B; /* luma width in units of blocks */
  const double   avgAct = sqrt (16.0 * (double)(1 << (2 * s->depth - 9)) / sqrt (MAX (0.00001, R))); /* = sqrt (a_pic) */
  const int*  strideOrg = (s->bpp == 1 ? s->planeWidth : s->lineSizes);
  uint32_t x, y, idxBlk = 0; /* the "16.0" above is due to fixed-point code */
  double* const sseLuma = s->sseLuma;
  double* const weights = s->weights;
  int c;
    
    if ((wsse64 == NULL) || (s->depth < 6) || (s->depth > 16)
            || (s->numComps <= 0) || (s->numComps > 3) || (W == 0)
            || (H == 0)) {
        printf("Error in XPSNR routine: invalid argument(s).\n");
        
        return -1;
    }
    
    if ((weights == NULL) || (B >= 4 && sseLuma == NULL)) {
        printf("Failed to allocate temporary block memory.\n");
        
        return -1;
    }

  if (B >= 4)
  {
    const bool blockWeightSmoothing = (W * H <= 640u * 480u); /* JITU paper */
    const FRAME_ELEM_TYPE *pOrg = org[0];
    const uint32_t sOrg = strideOrg[0] / s->bpp;
    const FRAME_ELEM_TYPE *pRec = rec[0];
    const uint32_t sRec = s->planeWidth[0];
    FRAME_ELEM_TYPE     *pOrgM1 = orgM1[0]; /* pixel  */
    FRAME_ELEM_TYPE     *pOrgM2 = orgM2[0]; /* memory */
    double wsseLuma = 0.0;

    for (y = 0; y < H; y += B) /* calculate block SSE and perceptual weight */
    {
      const uint32_t blockHeight = (y + B > H ? H - y : B);

      for (x = 0; x < W; x += B, idxBlk++)
      {
        const uint32_t blockWidth = (x + B > W ? W - x : B);
        double msAct = 1.0, msActPrev = 0.0;

        sseLuma[idxBlk] = calcSquaredErrorAndWeight(s, pOrg, sOrg,
                                                    pOrgM1, pOrgM2,
                                                    pRec, sRec,
                                                    x, y,
                                                    blockWidth, blockHeight,
                                                    s->depth, s->frameRate, &msAct);
        weights[idxBlk] = 1.0 / sqrt (msAct);

        if (blockWeightSmoothing) /* inline "minimum-smoothing" as in paper */
        {
          if (x == 0) /* first column */
          {
            msActPrev = (idxBlk > 1 ? weights[idxBlk - 2] : 0);
          }
          else  /* after first column */
          {
            msActPrev = (x > B ? MAX (weights[idxBlk - 2], weights[idxBlk]) : weights[idxBlk]);
          }
          if (idxBlk > WBlk) /* after first row and first column */
          {
            msActPrev = MAX (msActPrev, weights[idxBlk - 1 - WBlk]); /* min (left, top) */
          }
          if ((idxBlk > 0) && (weights[idxBlk - 1] > msActPrev))
          {
            weights[idxBlk - 1] = msActPrev;
          }
          if ((x + B >= W) && (y + B >= H) && (idxBlk > WBlk)) /* last block in picture */
          {
            msActPrev = MAX (weights[idxBlk - 1], weights[idxBlk - WBlk]);
            if (weights[idxBlk] > msActPrev)
            {
              weights[idxBlk] = msActPrev;
            }
          }
        }
      } /* for x */
    } /* for y */

    for (y = idxBlk = 0; y < H; y += B) /* calculate sum for luma (Y) XPSNR */
    {
      for (x = 0; x < W; x += B, idxBlk++)
      {
        wsseLuma += sseLuma[idxBlk] * weights[idxBlk];
      }
    }
    wsse64[0] = (wsseLuma <= 0.0 ? 0 : (uint64_t)(wsseLuma * avgAct + 0.5));
  } /* B >= 4 */

  for (c = 0; c < s->numComps; c++) /* finalize SSE data for all components */
  {
    const FRAME_ELEM_TYPE *pOrg = org[c];
    const uint32_t sOrg = strideOrg[c] / s->bpp;
    const FRAME_ELEM_TYPE *pRec = rec[c];
    const uint32_t sRec = s->planeWidth[c];
    const uint32_t WPln = s->planeWidth[c];
    const uint32_t HPln = s->planeHeight[c];

    if (B < 4) /* picture is too small for XPSNR, calculate unweighted PSNR */
    {
      wsse64[c] = calcSquaredError (pOrg, sOrg,
                                    pRec, sRec,
                                    WPln, HPln);
    }
    else if (c > 0) /* B >= 4, so Y XPSNR has already been calculated above */
    {
      const uint32_t Bx = (B * WPln) / W;
      const uint32_t By = (B * HPln) / H; /* up to chroma downsampling by 4 */
      double wsseChroma = 0.0;

      for (y = idxBlk = 0; y < HPln; y += By) /* calc. chroma (Cb/Cr) XPSNR */
      {
        const uint32_t blockHeight = (y + By > HPln ? HPln - y : By);

        for (x = 0; x < WPln; x += Bx, idxBlk++)
        {
          const uint32_t blockWidth = (x + Bx > WPln ? WPln - x : Bx);

          wsseChroma += (double) calcSquaredError(pOrg + y*sOrg + x, sOrg,
                                                  pRec + y*sRec + x, sRec,
                                                  blockWidth, blockHeight) * weights[idxBlk];
        }
      }
      wsse64[c] = (wsseChroma <= 0.0 ? 0 : (uint64_t)(wsseChroma * avgAct + 0.5));
    }
  } /* for c */

  return 0;
}

extern void
accum(XPSNRContext *s, XPSNR_FRAME *original, XPSNR_FRAME *recon, XPSNR_META *meta)
{
    int c, retValue;
    uint32_t W, H, B, WBlk, HBlk;
    FRAME_ELEM_TYPE *pOrg[3];
    FRAME_ELEM_TYPE *pOrgM1[3];
    FRAME_ELEM_TYPE *pOrgM2[3];
    FRAME_ELEM_TYPE *pRec[3];

    uint64_t wsse64[3] = { 0, 0, 0 };
    double curXPSNR[3] = { INFINITY, INFINITY, INFINITY };
    /* hardcoded for myself */
    s->bpp = 1;
    s->depth = 8;
#if 1
    s->maxError64 = (1 << s->depth) - 1; /* conventional limit */
#else
    s->maxError64 = 255 * (1 << (s->depth - 8)); /* JVET style */
#endif
    s->maxError64 *= s->maxError64;
    
    s->frameRate = meta->fps_num / meta->fps_den;
    s->numComps = 3;

    for (c = 0; c < 3; c++) {
        s->planeWidth[c] = original->planes[c].w;
        s->planeHeight[c] = original->planes[c].h;
    }
    /* unused */
    s->planeWidth[3] = original->planes[2].w;
    s->planeHeight[3] = original->planes[2].h;

    W = s->planeWidth[0]; /* luma image width in pixels */
    H = s->planeHeight[0]; /* luma image height in pixels */
    B = MAX(0, 4 * (int32_t )(32.0 * sqrt((double )(W * H) / (3840.0 * 2160.0)) + 0.5)); /* block size */
    WBlk = (W + B - 1) / B; /* luma width in units of blocks */
    HBlk = (H + B - 1) / B;/* luma height in units of blocks */

    /* prepare XPSNR calculation: allocate temporary picture and block memory */
    if (s->sseLuma == NULL)
        s->sseLuma = (double*) xpsnr_alloc(WBlk * HBlk, sizeof(double));
    if (s->weights == NULL)
        s->weights = (double*) xpsnr_alloc(WBlk * HBlk, sizeof(double));
    
    for (c = 0; c < s->numComps; c++) /* allocate temporal org buffer memory */
    {
        s->lineSizes[c] = original->planes[c].stride;
        
        if (c == 0) /* luma ch. */
        {
            const int strideOrgBpp = (
                    s->bpp == 1 ? s->planeWidth[c] : s->lineSizes[c] / s->bpp);
            
            if (s->bufOrgM1[c] == NULL)
                s->bufOrgM1[c] = xpsnr_allocz(
                        strideOrgBpp * s->planeHeight[c] * sizeof(FRAME_ELEM_TYPE));
            if (s->bufOrgM2[c] == NULL)
                s->bufOrgM2[c] = xpsnr_allocz(
                        strideOrgBpp * s->planeHeight[c] * sizeof(FRAME_ELEM_TYPE));
            
            pOrgM1[c] = (FRAME_ELEM_TYPE*) s->bufOrgM1[c];
            pOrgM2[c] = (FRAME_ELEM_TYPE*) s->bufOrgM2[c];
        }
    }
    
    if (s->bpp == 1) /* 8 bit */
    {
        int x, y;
        for (c = 0; c < s->numComps; c++) /* allocate the org/rec buffer memory */
        {
            const int M = s->lineSizes[c]; /* original stride */
            const int R = recon->planes[c].stride; /* recon/c stride */
            const int O = s->planeWidth[c]; /* XPSNR stride */
            
            if (s->bufOrg[c] == NULL)
                s->bufOrg[c] = xpsnr_allocz(
                        s->planeWidth[c] * s->planeHeight[c] * sizeof(FRAME_ELEM_TYPE));
            if (s->bufRec[c] == NULL)
                s->bufRec[c] = xpsnr_allocz(
                        s->planeWidth[c] * s->planeHeight[c] * sizeof(FRAME_ELEM_TYPE));
            
            pOrg[c] = (FRAME_ELEM_TYPE*) s->bufOrg[c];
            pRec[c] = (FRAME_ELEM_TYPE*) s->bufRec[c];

            for (y = 0; y < s->planeHeight[c]; y++) {
                for (x = 0; x < s->planeWidth[c]; x++) {
                    pOrg[c][y * O + x] = (FRAME_ELEM_TYPE) original->planes[c].data[y * M + x];
                    pRec[c][y * O + x] = (FRAME_ELEM_TYPE) recon->planes[c].data[y * R + x];
                }
            }
        }
    } else /* 10, 12, or 14 bit */
    {
        for (c = 0; c < s->numComps; c++) {
            pOrg[c] = (FRAME_ELEM_TYPE*) original->planes[c].data;
            pRec[c] = (FRAME_ELEM_TYPE*) recon->planes[c].data;
        }
    }
    /* extended perceptually weighted peak signal-to-noise ratio (XPSNR) data */

    if ((retValue = getWSSE(s, (FRAME_ELEM_TYPE**) &pOrg, (FRAME_ELEM_TYPE**) &pOrgM1,
            (FRAME_ELEM_TYPE**) &pOrgM2, (FRAME_ELEM_TYPE**) &pRec, wsse64)) < 0) {
        printf("error near end of xpsnr!\n");
        return; /* an error here implies something went wrong earlier! */
    }
    
    for (c = 0; c < s->numComps; c++) {
        const double sqrtWSSE = sqrt((double) wsse64[c]);
        
        curXPSNR[c] = getAvgXPSNR(sqrtWSSE, INFINITY, s->planeWidth[c],
                s->planeHeight[c], s->maxError64, 1 /* single frame */);
        s->sumWDist[c] += sqrtWSSE;
        s->sumXPSNR[c] += curXPSNR[c];
        s->andIsInf[c] &= isinf(curXPSNR[c]);
    }
}
