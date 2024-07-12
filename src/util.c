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

#include "util.h"
#include <stdlib.h>
#include <string.h>

#define FINISHED_TAGS 2
static int
read_token(FILE *in, char *line, char delim)
{
    int i;
    int c, ret = 1;
    
    i = 0;
    c = fgetc(in);
    while (c != delim) {
        if (c == EOF) {
            return 0;
        }
        if (c == '\n') {
            ret = FINISHED_TAGS;
            break;
        }
        if (i >= 255) {
            fprintf(stderr, "Y4M parse error!\n");
            return 0;
        }
        line[i] = c;
        i++;
        c = fgetc(in);
    }
    line[i] = '\0';
    return ret;
}

extern int
dsv_y4m_read_hdr(FILE *in, int *w, int *h, int *subsamp, int *framerate)
{
#define Y4M_HDR "YUV4MPEG2 "
#define Y4M_TAG_DELIM 0x20
#define Y4M_EARLY_EOF \
        do if (res == 0) { \
            fprintf(stderr, "parsing Y4M: early EOF\n"); \
            return 0; \
        } while (0)
    char line[256];
    int interlace = 0;
    int c = 0, res;
    if (fread(line, 1, sizeof(Y4M_HDR) - 1, in) != (sizeof(Y4M_HDR) - 1)) {
        fprintf(stderr, "Bad Y4M header\n");
        return 0;
    }
    c = fgetc(in);
    *subsamp = DSV_SUBSAMP_420; /* default */
    while (c != '\n') {
        switch (c) {
            case 'W':
                res = read_token(in, line, Y4M_TAG_DELIM);
                Y4M_EARLY_EOF;
                
                *w = atoi(line);
                if (*w <= 0) {
                    fprintf(stderr, "parsing Y4M: bad width %d\n", *w);
                    return 0;
                }
                if (res == FINISHED_TAGS) {
                    goto done;
                }
                break;
            case 'H':
                res = read_token(in, line, Y4M_TAG_DELIM);
                Y4M_EARLY_EOF;

                *h = atoi(line);
                if (*h <= 0) {
                    fprintf(stderr, "parsing Y4M: bad height %d\n", *h);
                    return 0;
                }
                if (res == FINISHED_TAGS) {
                    goto done;
                }
                break;
            case 'F':
                if (!read_token(in, line, ':')) {
                    fprintf(stderr, "parsing Y4M: early EOF\n");
                    return 0;
                }
                framerate[0] = atoi(line);
                res = read_token(in, line, Y4M_TAG_DELIM);
                Y4M_EARLY_EOF;

                framerate[1] = atoi(line);
                if (res == FINISHED_TAGS) {
                    goto done;
                }
                break;
            case 'I':
                res = read_token(in, line, Y4M_TAG_DELIM);
                Y4M_EARLY_EOF;

                interlace = line[0];
                if (res == FINISHED_TAGS) {
                    goto done;
                }
                break;
            case 'A':
                if (!read_token(in, line, ':')) {
                    fprintf(stderr, "parsing Y4M: early EOF\n");
                    return 0;
                }
               /* aspect[0] = atoi(line); */
                res = read_token(in, line, Y4M_TAG_DELIM);
                Y4M_EARLY_EOF;

               /* aspect[1] = atoi(line); */
                if (res == FINISHED_TAGS) {
                    goto done;
                }
                break;
            case 'C':
                res = read_token(in, line, Y4M_TAG_DELIM);
                Y4M_EARLY_EOF;
               
                if (line[0] == '4' && line[1] == '2' && line[2] == '0') {
                    *subsamp = DSV_SUBSAMP_420;
                } else if (line[0] == '4' && line[1] == '1' && line[2] == '1') {
                    *subsamp = DSV_SUBSAMP_411;
                } else if (line[0] == '4' && line[1] == '2' && line[2] == '2') {
                    *subsamp = DSV_SUBSAMP_422;
                } else if (line[0] == '4' && line[1] == '4' && line[2] == '4') {
                    *subsamp = DSV_SUBSAMP_444;
                } else {
                    fprintf(stderr, "Bad Y4M subsampling: %s\n", line);
                }
                if (res == FINISHED_TAGS) {
                    goto done;
                }
                break;
            case 'X':
                res = read_token(in, line, Y4M_TAG_DELIM);
                Y4M_EARLY_EOF;

                if (res == FINISHED_TAGS) {
                    goto done;
                }
                break;
        }
        c = fgetc(in);
    }
done:
    if (interlace != 'p') {
        fprintf(stderr, "interlaced video likely causes issues with XPSNR\n");
    }
    return 1;
}

extern int
dsv_y4m_read_seq(FILE *in, uint8_t *o, int w, int h, int subsamp)
{
    size_t npix, chrsz = 0;
#define Y4M_FRAME_HDR "FRAME\n"
    size_t hdrsz;
    char line[8];
    hdrsz = sizeof(Y4M_FRAME_HDR) - 1;
    
    if (in == NULL) {
        return -1;
    }
    if (fread(line, 1, hdrsz, in) != hdrsz) {
        return -1;
    }
    if (memcmp(line, Y4M_FRAME_HDR, hdrsz) != 0) {
        fprintf(stderr, "bad Y4M frame header [%s]\n", line);
        return -1;
    }
    npix = w * h;
    switch (subsamp) {
        case DSV_SUBSAMP_444:
            chrsz = npix;
            break;
        case DSV_SUBSAMP_422:
            chrsz = (w / 2) * h;
            break;
        case DSV_SUBSAMP_420:
        case DSV_SUBSAMP_411:
            chrsz = npix / 4;
            break;
        default:
            fprintf(stderr, "unsupported format %d\n", subsamp);
            break;
    }
    if (fread(o, 1, npix + chrsz + chrsz, in) != (npix + chrsz + chrsz)) {
        return -1;
    }
    return 0;
}

extern int
dsv_yuv_read_seq(FILE *in, uint8_t *o, int width, int height, int subsamp)
{
    size_t npix, chrsz = 0;
    
    if (in == NULL) {
        return -1;
    }
    npix = width * height;
    switch (subsamp) {
        case DSV_SUBSAMP_444:
            chrsz = npix;
            break;
        case DSV_SUBSAMP_422:
            chrsz = (width / 2) * height;
            break;
        case DSV_SUBSAMP_420:
        case DSV_SUBSAMP_411:
            chrsz = npix / 4;
            break;
        default:
            fprintf(stderr, "unsupported format %d\n", subsamp);
            break;
    }
    if (fread(o, 1, npix + chrsz + chrsz, in) != (npix + chrsz + chrsz)) {
        return -1;
    }
    return 0;
}
