# Standalone XPSNR (sxpsnr)
Standalone XPSNR command line utility for calculating the XPSNR metric on two video sources

## Usage

```bash
Standalone XPSNR CLI | v1.0.1

Usage:
	sxpsnr [options]

Parameters:
	-w= : width of input video. 352 = default
	      [min = 16, max = 16777216]
	-h= : height of input video. 288 = default
	      [min = 16, max = 16777216]
	-fmt= : chroma subsampling format of input video. 0 = 4:4:4, 1 = 4:2:2, 2 = 4:2:0, 3 = 4:1:1, 2 = default
	      [min = 0, max = 3]
	-nfr= : number of frames to compress. -1 means as many as possible. -1 = default
	      [min = -1, max = 2147483647]
	-fps_num= : fps numerator of input video. 30 = default
	      [min = 1, max = 16777216]
	-fps_den= : fps denominator of input video. 1 = default
	      [min = 1, max = 16777216]
	-y4m= : set to 1 if input is in Y4M format, 0 if raw YUV. 0 = default
	      [min = 0, max = 1]
	-dst= : distorted input file.
	-ref= : reference input file.
	-v    : set verbose
Sample usage: sxpsnr -dst=decoded.y4m -ref=original.y4m -y4m=1
Sample usage: sxpsnr -dst=decoded.yuv -ref=original.yuv -w=352 -h=288 -fmt=2 -fps_num=30
```

## Installation

`sxpsnr` can be easily built for your system using the Zig build system. Building requires Zig version ≥`0.13.0`.

0. Ensure you have Zig & git installed

1. Clone this repo & enter the cloned directory:

```bash
git clone https://github.com/gianni-rosato/sxpsnr.git
cd sxpsnr
```

2. Build the binary with Zig:

```bash
zig build
```
> Note: If you'd like to specify a different build target from your host OS/architecture, simply supply the target flag. Example: `zig build -Dtarget=x86_64-linux-gnu`

3. Find the build binary in `zig-out/bin`. You can install it like so:

```bash
sudo cp zig-out/bin/sxpsnr /usr/local/bin
```

Now, you should be all set to use `sxpsnr`.
