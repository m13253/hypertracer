const std = @import("std");
const cborlite = @import("cborlite.zig");

pub fn convert(allocator: std.mem.Allocator, filename_in: []const []const u8, filename_out: ?[]const u8) !void {
    var fout: ?std.fs.File = null;
    var fout_buf: ?@TypeOf(std.io.bufferedWriter(fout.?.writer())) = null;
    var fout_need_close = false;
    defer {
        if (fout_buf) |*f| {
            f.flush() catch {};
        }
        if (fout_need_close) {
            fout.?.close();
        }
    }
    for (filename_in) |i| {
        const fin, const fin_need_close = if (std.mem.eql(u8, i, "-"))
            .{ std.io.getStdIn(), false }
        else
            .{ try std.fs.cwd().openFile(i, .{}), true };
        defer if (fin_need_close) {
            fin.close();
        };
        var fin_buf = std.io.bufferedReader(fin.reader());

        var arena = std.heap.ArenaAllocator.init(allocator);
        defer arena.deinit();
        var parser = cborlite.Parser(@TypeOf(fin_buf).Reader).init(arena.allocator(), fin_buf.reader());
        while (try parser.nextValue()) |value| {
            defer _ = arena.reset(.retain_capacity);
            switch (value) {
                .stream_array_start, .break_mark => continue,
                else => {},
            }
            const v1 = try value.clone(allocator);
            v1.deinit(allocator);
            std.debug.print("{any}\n", .{value});
        }

        if (fout == null) {
            if (filename_out == null or std.mem.eql(u8, filename_out.?, "-")) {
                fout = std.io.getStdOut();
                fout_need_close = false;
            } else {
                fout = try std.fs.cwd().createFile(filename_out.?, .{});
                fout_need_close = true;
            }
            fout_buf = std.io.bufferedWriter(fout.?.writer());
        }
        try fout_buf.?.flush();
    }
}
