const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});

    // Create the executable
    const bin = b.addExecutable(.{
        .name = "sxpsnr",
        .target = target,
        .optimize = .ReleaseFast,
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
            "-O3",
        },
    });

    // Link libc
    bin.linkLibC();
    b.installArtifact(bin);
}
