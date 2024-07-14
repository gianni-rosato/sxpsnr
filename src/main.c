/*****************************************************************************/
/*
 * Main file for XPSNR command line driver.
 *
 * Driver written by EMMIR (LMP88959)
 * some code by Gianni Rosato (not much)
 * XPSNR written by Christian Helmrich and Christian Stoffers
 *
 * Driver (main.c, util.c, util.h) is public domain.
 */
/*****************************************************************************/
#include "xpsnr.h"
#include "util.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#define DRV_VERSION "1.0.1"
#define DRV_HEADER "Standalone XPSNR CLI | \x1b[36mv"DRV_VERSION"\x1b[0m\n"

static char *progname = NULL;
static int verbose = 0;

#define INP_FMT_444 0
#define INP_FMT_422 1
#define INP_FMT_420 2
#define INP_FMT_411 3

#ifndef CLAMP
#define CLAMP(x, a, b) ((x) < (a) ? (a) : ((x) > (b) ? (b) : (x)))
#endif

static int
fmt_to_subsamp(int fmt)
{
    switch (fmt) {
        case INP_FMT_444:
            return DSV_SUBSAMP_444;
        case INP_FMT_422:
            return DSV_SUBSAMP_422;
        case INP_FMT_420:
            return DSV_SUBSAMP_420;
        case INP_FMT_411:
            return DSV_SUBSAMP_411;
    }
    return DSV_SUBSAMP_420;
}

struct PARAM {
   char *prefix;
   int value;
   int min, max;
   int (*convert)(int);
   char *desc;
};

static struct PARAM dec_params[] = {
    { "w=", 352, 16, (1 << 24), NULL,
            "width of input video. 352 = default" },
    { "h=", 288, 16, (1 << 24), NULL,
            "height of input video. 288 = default" },
    { "fmt=", DSV_SUBSAMP_420, 0, 3, fmt_to_subsamp,
            "chroma subsampling format of input video. 0 = 4:4:4, 1 = 4:2:2, 2 = 4:2:0, 3 = 4:1:1, 2 = default" },
    { "nfr=", -1, -1, INT_MAX, NULL,
            "number of frames to compress. -1 means as many as possible. -1 = default" },
    { "fps_num=", 30, 1, (1 << 24), NULL,
            "fps numerator of input video. 30 = default" },
    { "fps_den=", 1, 1, (1 << 24), NULL,
            "fps denominator of input video. 1 = default" },
    { "y4m=", 0, 0, 1, NULL,
            "set to 1 if input is in Y4M format, 0 if raw YUV. 0 = default" },
    { NULL, 0, 0, 0, NULL, "" }
};

static struct {
   char *inp_dec;
   char *inp_ref;
} opts;

static int
get_optval(struct PARAM *pars, char *name)
{
    int i;
    for (i = 0; pars[i].prefix != NULL; i++) {
        struct PARAM *par = &pars[i];
        if (strcmp(par->prefix, name) == 0) {
            return par->value;
        }
    }
    return 0;
}

static void
print_params(struct PARAM *pars)
{
    int i;

    printf("\n");
    for (i = 0; pars[i].prefix != NULL; i++) {
        struct PARAM *par = &pars[i];

        printf("\t-%s : %s\n", par->prefix, par->desc);
        printf("\t      [min = %d, max = %d]\n", par->min, par->max);
    }
    printf("\t-dst= : distorted input file.\n");
    printf("\t-ref= : reference input file.\n");
    printf("\t-v    : set verbose\n");
}

static void
sample_usage(char* p)
{
    printf("\x1b[2mSample usage: %s -dst=decoded.y4m -ref=original.y4m -y4m=1\x1b[0m\n", p);
    printf("\x1b[2mSample usage: %s -dst=decoded.yuv -ref=original.yuv -w=352 -h=288 -fmt=2 -fps_num=30\x1b[0m\n", p);
}

static void
usage(void)
{
    char *p = progname;

    printf(DRV_HEADER);
    printf("\n\x1b[4mUsage\x1b[0m:\n\t%s [\x1b[36moptions\x1b[0m]\n\n", p);
    printf("\x1b[4mParameters\x1b[0m:");
    print_params(dec_params);
    sample_usage(p);
}

static int
stoint(char *s, int *err)
{
    char *tail;
    long val;

    errno = 0;
    *err = 0;
    val = strtol(s, &tail, 10);
    if (errno == ERANGE) {
        fprintf(stderr, "\x1b[31mError: Integer out of integer range\x1b[0m\n");
        *err = 1;
    } else if (errno != 0) {
        fprintf(stderr, "\x1b[31mError: Bad string: %s\x1b[0m\n", strerror(errno));
        *err = 1;
    } else if (*tail != '\0') {
        fprintf(stderr, "\x1b[31mError: Integer contained non-numeric characters\x1b[0m\n");
        *err = 1;
    }
    return val;
}

static int
prefixcmp(char *pref, char **s)
{
    int plen = strlen(pref);
    if (!strncmp(pref, *s, plen)) {
        *s += plen;
        return 1;
    }
    return 0;
}

static int
get_param(char *argv)
{
    int i;
    char *p = argv;
    int err = 0;
    struct PARAM *params;

    if (*p != '-') {
        fprintf(stderr, "\x1b[33mStrange argument: %s\x1b[0m\n", p);
        return 0;
    }

    p++;
    if (strcmp("v", p) == 0) {
        verbose = 1;
        return 1;
    }
    if (prefixcmp("dst=", &p)) {
        opts.inp_dec = p;
        return 1;
    }
    if (prefixcmp("ref=", &p)) {
        opts.inp_ref = p;
        return 1;
    }
    params = dec_params;
    for (i = 0; params[i].prefix != NULL; i++) {
        struct PARAM *par = &params[i];
        if (!prefixcmp(par->prefix, &p)) {
            continue;
        }
        par->value = CLAMP(par->value, par->min, par->max);
        par->value = par->convert ? par->convert(stoint(p, &err)) : stoint(p, &err);
        if (err) {
            fprintf(stderr, "\x1b[31mError reading argument: \"%s\"\x1b[0m\n", par->prefix);
            return 0;
        }
        return 1;
    }
    fprintf(stderr, "\x1b[31mError: Unrecognized argument(s)\x1b[0m\n");
    return 0;
}

static int
init_params(int argc, char **argv)
{
    int i;

    if (argc == 1) {
        fprintf(stderr, "\x1b[31mError: Insufficient argument count\x1b[0m\n");
        usage();
        return 0;
    }
    argc--;
    argv++;
    while (argc > 0) {
        i = get_param(*argv);
        if (i == 0) {
            usage();
            return 0;
        }
        argv += i;
        argc -= i;
    }
    return 1;
}

static XPSNR_FRAME *
load_planar_frame(int format, void *data, int width, int height)
{
    XPSNR_FRAME *f;
    int hs, vs;

    f = xpsnr_allocz(sizeof(XPSNR_FRAME));
    hs = DSV_FORMAT_H_SHIFT(format);
    vs = DSV_FORMAT_V_SHIFT(format);

    f->planes[0].format = format;
    f->planes[0].w = width;
    f->planes[0].h = height;
    f->planes[0].stride = width;
    f->planes[0].data = data;
    f->planes[0].len = f->planes[0].stride * f->planes[0].h;

    width = DSV_ROUND_SHIFT(width, hs);
    height = DSV_ROUND_SHIFT(height, vs);

    f->planes[1].format = format;
    f->planes[1].w = width;
    f->planes[1].h = height;
    f->planes[1].stride = f->planes[1].w;
    f->planes[1].len = f->planes[1].stride * f->planes[1].h;
    f->planes[1].data = f->planes[0].data + f->planes[0].len;

    f->planes[2].format = format;
    f->planes[2].w = width;
    f->planes[2].h = height;
    f->planes[2].stride = f->planes[2].w;
    f->planes[2].len = f->planes[2].stride * f->planes[2].h;
    f->planes[2].data = f->planes[1].data + f->planes[1].len;
    return f;
}

static int
readframes(void)
{
    XPSNR_META md;
    uint32_t frno = 0;
    int w, h;
    FILE *decfile, *reffile;
    int y4m_in = 0;
    int maxframe, nfr;
    uint8_t *refdata;
    uint8_t *decdata;
    XPSNRContext xpctx;
    double lxp, uxp, vxp, yuvxp, wxp, hm;

    decfile = fopen(opts.inp_dec, "rb");
    if (decfile == NULL) {
        fprintf(stderr, "error opening input file %s\n", opts.inp_dec);
        return EXIT_FAILURE;
    }
    reffile = fopen(opts.inp_ref, "rb");
    if (reffile == NULL) {
        fprintf(stderr, "error opening input file %s\n", opts.inp_ref);
        return EXIT_FAILURE;
    }

    w = get_optval(dec_params, "w=");
    h = get_optval(dec_params, "h=");
    md.width = w;
    md.height = h;
    md.subsamp = get_optval(dec_params, "fmt=");
    md.fps_num = get_optval(dec_params, "fps_num=");
    md.fps_den = get_optval(dec_params, "fps_den=");

    y4m_in = get_optval(dec_params, "y4m=");
    if (y4m_in) {
        int fr[2] = { 1, 1 };

        if (!dsv_y4m_read_hdr(reffile, &md.width, &md.height, &md.subsamp, fr)) {
            fprintf(stderr, "(ref) bad Y4M file %s\n", opts.inp_ref);
            return EXIT_FAILURE;
        }
        w = md.width;
        h = md.height;
        if (!dsv_y4m_read_hdr(decfile, &md.width, &md.height, &md.subsamp, fr)) {
            fprintf(stderr, "(dec) bad Y4M file %s\n", opts.inp_dec);
            return EXIT_FAILURE;
        }
        if (w != md.width || h != md.height) {
            fprintf(stderr, "dst & ref dimensions do not match! %dx%d vs %dx%d\n", md.width, md.height, w, h);
            return EXIT_FAILURE;
        }
        md.fps_num = fr[0];
        md.fps_den = fr[1];
        if (md.fps_den <= 0) {
            fprintf(stderr, "fps denominator was <= 0. Setting to 1.");
            md.fps_den = 1;
        }
    }

#define EXTRA_PAD 1
    decdata = xpsnr_allocz(w * h * (3 + EXTRA_PAD)); /* allocate extra to be safe */
    refdata = xpsnr_allocz(w * h * (3 + EXTRA_PAD));

    nfr = get_optval(dec_params, "nfr=");
    if (nfr > 0) {
        maxframe = frno + nfr;
    } else {
        maxframe = -1;
    }
    if (verbose) {
        printf("%s video | ", y4m_in ? "YUV4MPEG2" : "Raw YUV");
        printf("%dx%d @ %d/%d frames per second | ", w, h, md.fps_num, md.fps_den);
        switch (md.subsamp) {
            case DSV_SUBSAMP_444:
                puts("Planar YUV 4:4:4");
                break;
            case DSV_SUBSAMP_422:
                puts("Planar YUV 4:2:2");
                break;
            case DSV_SUBSAMP_420:
                puts("Planar YUV 4:2:0");
                break;
            case DSV_SUBSAMP_411:
                puts("Planar YUV 4:1:1");
                break;
        }
    }
    puts("Calculating XPSNR...");
    memset(&xpctx, 0, sizeof(xpctx));

    while (1) {
        XPSNR_FRAME *decf, *reff;
        if (maxframe > 0 && frno >= (unsigned) maxframe) {
            break;
        }
        if (y4m_in) {
            if (dsv_y4m_read_seq(decfile, decdata, w, h, md.subsamp) < 0) {
                break;
            }
            if (dsv_y4m_read_seq(reffile, refdata, w, h, md.subsamp) < 0) {
                break;
            }
        } else {
            if (dsv_yuv_read_seq(decfile, decdata, w, h, md.subsamp) < 0) {
                break;
            }
            if (dsv_yuv_read_seq(reffile, refdata, w, h, md.subsamp) < 0) {
                break;
            }
        }
        decf = load_planar_frame(md.subsamp, decdata, w, h);
        reff = load_planar_frame(md.subsamp, refdata, w, h);
        /* compute metrics and accumulate */
        accum(&xpctx, reff, decf, &md);

        xpsnr_free(reff);
        xpsnr_free(decf);
        xpctx.numFrames64++;
        frno++;
    }
    lxp = getAvgXPSNR(xpctx.sumWDist[0], xpctx.sumXPSNR[0],
            xpctx.planeWidth[0], xpctx.planeHeight[0], xpctx.maxError64,
            xpctx.numFrames64);
    uxp = getAvgXPSNR(xpctx.sumWDist[1], xpctx.sumXPSNR[1],
            xpctx.planeWidth[1], xpctx.planeHeight[1], xpctx.maxError64,
            xpctx.numFrames64);
    vxp = getAvgXPSNR(xpctx.sumWDist[2], xpctx.sumXPSNR[2],
            xpctx.planeWidth[2], xpctx.planeHeight[2], xpctx.maxError64,
            xpctx.numFrames64);
    yuvxp = (lxp + uxp + vxp) / 3.0;
    wxp = ((lxp * 4.0) + uxp + vxp) / 6.0;
    hm = 3.0 / ((1.0 / lxp) + (1.0 / uxp) + (1.0 / vxp));
    printf("---\n");
    printf("XPSNR Y \t= %f | XPSNR YUV\t\t= %f\n", lxp, yuvxp);
    printf("XPSNR U \t= %f | HarmMean YUV\t= %f\n", uxp, hm);
    printf("XPSNR V \t= %f | Weighted XPSNR\t= %f\n", vxp, wxp);

    fclose(decfile);
    fclose(reffile);
    xpsnr_free(decdata);
    xpsnr_free(refdata);

    return EXIT_SUCCESS;
}

static int
startup(int argc, char **argv)
{
    if (!init_params(argc, argv)) {
        return EXIT_SUCCESS;
    }
    if (!opts.inp_dec || !opts.inp_ref) {
        fprintf(stderr, "dst= or ref= was not specified!\n");
        usage();
        return EXIT_FAILURE;
    }

    return readframes();
}

int
main(int argc, char **argv)
{
    progname = argv[0];
    return startup(argc, argv);
}
