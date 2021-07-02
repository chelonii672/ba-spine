const std = @import("std");

pub fn build(b: *std.build.Builder) void {
    const target = b.standardTargetOptions(.{});
    const mode = b.standardReleaseOptions();

    const exe = b.addExecutable("ba-spine", "src/main.zig");
    exe.setTarget(target);
    exe.setBuildMode(mode);

    exe.strip = b.option(bool, "strip", "Omit debug symbols");

    exe.install();
}
