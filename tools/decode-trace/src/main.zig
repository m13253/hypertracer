const std = @import("std");
const argparser = @import("argparser.zig");
const cborlite = @import("cborlite.zig");
const converter = @import("converter.zig");

pub const std_options = std.Options{
    .fmt_max_depth = std.math.maxInt(usize),
};

pub fn main() !u8 {
    var gpa = std.heap.GeneralPurposeAllocator(.{}).init;
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();

    var argsIterator = try std.process.argsWithAllocator(allocator);
    defer argsIterator.deinit();
    const args = argparser.ArgParser.init(allocator, &argsIterator) catch |err| {
        if (err == error.ArgParseError) {
            return 1;
        }
        return err;
    } orelse return 0;
    defer args.deinit();

    try converter.convert(allocator, args.filename_in, args.filename_out);

    return 0;
}
