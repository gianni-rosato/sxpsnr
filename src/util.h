/*****************************************************************************/
/*
 * YUV/Y4M loader for XPSNR command line driver.
 * 
 * Driver written by EMMIR (LMP88959)
 * XPSNR written by Christian Helmrich and Christian Stoffers
 * 
 * Driver (main.c, util.c, util.h) is public domain.
 */
/*****************************************************************************/

#ifndef _UTIL_H_
#define _UTIL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>

#define DSV_FMT_FULL_V 0x0
#define DSV_FMT_DIV2_V 0x1
#define DSV_FMT_DIV4_V 0x2
#define DSV_FMT_FULL_H 0x0
#define DSV_FMT_DIV2_H 0x4
#define DSV_FMT_DIV4_H 0x8

#define DSV_SUBSAMP_444  (DSV_FMT_FULL_H | DSV_FMT_FULL_V)
#define DSV_SUBSAMP_422  (DSV_FMT_DIV2_H | DSV_FMT_FULL_V)
#define DSV_SUBSAMP_420  (DSV_FMT_DIV2_H | DSV_FMT_DIV2_V)
#define DSV_SUBSAMP_411  (DSV_FMT_DIV4_H | DSV_FMT_FULL_V)

#define DSV_FORMAT_H_SHIFT(format) (((format) >> 2) & 0x3)
#define DSV_FORMAT_V_SHIFT(format) ((format) & 0x3)
#define DSV_ROUND_SHIFT(x, shift) (((x) + (1 << (shift)) - 1) >> (shift))

extern int dsv_y4m_read_hdr(FILE *in, int *w, int *h, int *subs, int *frmrate);
extern int dsv_y4m_read_seq(FILE *in, uint8_t *o, int w, int h, int subsamp);
extern int dsv_yuv_read_seq(FILE *in, uint8_t *o, int w, int h, int subsamp);

#ifdef __cplusplus
}
#endif

#endif
