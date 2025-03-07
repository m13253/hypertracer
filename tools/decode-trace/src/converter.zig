const std = @import("std");
const cborlite = @import("cborlite.zig");
const jsonlite = @import("jsonlite.zig");
const ObjCache = @import("objcache.zig").ObjCache;

pub fn convert(allocator: std.mem.Allocator, filename_in: []const []const u8, filename_out: ?[]const u8) !void {
    var fout: std.fs.File = undefined;
    var fout_buf: ?@TypeOf(std.io.bufferedWriter(fout.writer())) = null;
    var json_writer: jsonlite.Writer(@TypeOf(fout_buf.?.writer())) = undefined;
    var fout_need_close = false;
    var comma = false;
    defer {
        if (fout_buf) |*f| {
            f.flush() catch {};
        }
        if (fout_need_close) {
            fout.close();
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

        var objcache = ObjCache.init(allocator);
        defer objcache.deinit();

        var parser_arena = std.heap.ArenaAllocator.init(allocator);
        defer _ = parser_arena.deinit();
        var parser = cborlite.Parser(@TypeOf(fin_buf).Reader).init(parser_arena.allocator(), fin_buf.reader());

        while (try parser.nextValue()) |value| {
            defer _ = parser_arena.reset(.retain_capacity);
            //defer value.deinit(parser_arena.allocator());

            if (fout_buf == null) {
                if (filename_out == null or std.mem.eql(u8, filename_out.?, "-")) {
                    fout = std.io.getStdOut();
                    fout_need_close = false;
                } else {
                    fout = try std.fs.cwd().createFile(filename_out.?, .{});
                    fout_need_close = true;
                }
                fout_buf = std.io.bufferedWriter(fout.writer());
                try fout_buf.?.writer().writeByte('[');
                json_writer = jsonlite.Writer(@TypeOf(fout_buf.?.writer())).init(fout_buf.?.writer());
            }

            switch (value) {
                .stream_array_start, .break_mark => continue,
                else => {
                    const json_out = try objcache.processRecord(value);
                    if (json_out) |json_out_| {
                        defer json_out_.deinit(allocator);
                        if (comma) {
                            try fout_buf.?.writer().writeAll(",\n");
                        } else {
                            try fout_buf.?.writer().writeByte('\n');
                        }
                        comma = true;
                        try json_writer.write(json_out_);
                    }
                },
            }
        }
    }
    if (fout_buf) |*f| {
        try f.writer().writeAll("\n]\n");
        try f.flush();
    }
}
