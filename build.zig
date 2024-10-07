const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{ .preferred_optimize_mode = .ReleaseFast });
    const strip = b.option(bool, "strip", "Strip symbols from the binary. Default: false") orelse false;

    // Create the executable
    const bin = b.addExecutable(.{
        .name = "sxpsnr",
        .target = target,
        .link_libc = true,
        .optimize = optimize,
        .strip = strip,
    });

    // Add C source files
    bin.addCSourceFiles(.{
        .files = &.{
            "src/main.c",
            "src/util.c",
            "src/xpsnr.c",
        },
        .flags = &.{
            "-std=c99",
            "-lm",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
        },
    });

    b.installArtifact(bin);
}
