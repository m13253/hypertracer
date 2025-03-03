const std = @import("std");
const argparser = @import("argparser.zig");
const cborlite = @import("cborlite.zig");

pub fn main() !u8 {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();

    var argsIterator = try std.process.argsWithAllocator(allocator);
    defer argsIterator.deinit();
    const args = argparser.ArgParser.init(allocator, &argsIterator) catch |err| {
        if (err == error.ArgParseError) {
            return 1;
        }
        return err;
    };
    defer args.deinit();

    return 0;
}
