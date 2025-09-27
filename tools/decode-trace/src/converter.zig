const std = @import("std");
const cborlite = @import("cborlite.zig");
const jsonlite = @import("jsonlite.zig");
const ObjCache = @import("objcache.zig").ObjCache;

const DefaultBufferSize = 4096;

pub fn convert(allocator: std.mem.Allocator, filename_in: []const []const u8, filename_out: ?[]const u8) !void {
    var fout_buffer: [DefaultBufferSize]u8 = undefined;
    var fout: ?std.fs.File = null;
    var fout_writer: ?std.fs.File.Writer = null;
    var json_writer: jsonlite.Writer = undefined;
    var comma = false;
    defer if (fout) |*f| {
        f.close();
    };
    errdefer if (fout_writer) |*f| {
        f.interface.flush() catch {};
    };
    for (filename_in) |i| {
        var fin_buffer: [DefaultBufferSize]u8 = undefined;
        var fin: ?std.fs.File = null;
        var fin_reader: std.fs.File.Reader = undefined;

        if (std.mem.eql(u8, i, "-")) {
            fin_reader = std.fs.File.stdin().reader(&fin_buffer);
        } else {
            fin = try std.fs.cwd().openFile(i, .{});
            fin_reader = fin.?.reader(&fin_buffer);
        }
        defer if (fin) |*f| {
            f.close();
        };

        var objcache = ObjCache.init();
        defer objcache.deinit(allocator);

        var parser_arena = std.heap.ArenaAllocator.init(allocator);
        defer _ = parser_arena.deinit();
        const parser_allocator = parser_arena.allocator();
        var parser = cborlite.Parser.init(&fin_reader.interface);

        blk: {
            while (parser.nextValue(parser_allocator) catch |err| {
                const root_cause = if (err == std.Io.Writer.Error.WriteFailed)
                    fin_reader.err orelse err
                else
                    err;
                std.debug.print("Error: File {s} at position 0x{x}: {s}\n", .{ i, parser.bytesRead(), @errorName(root_cause) });
                break :blk;
            }) |value| {
                defer _ = parser_arena.reset(.retain_capacity);
                //defer value.deinit(parser_arena.allocator());

                if (fout_writer == null) {
                    if (filename_out == null or std.mem.eql(u8, filename_out.?, "-")) {
                        fout_writer = std.fs.File.stdout().writer(&fout_buffer);
                    } else {
                        fout = try std.fs.cwd().createFile(filename_out.?, .{});
                        fout_writer = fout.?.writer(&fout_buffer);
                    }
                    try fout_writer.?.interface.writeByte('[');
                    json_writer = jsonlite.Writer.init(&fout_writer.?.interface);
                }

                switch (value) {
                    .stream_array_start, .break_mark => continue,
                    else => {
                        const json_out = try objcache.processRecord(allocator, value);
                        if (json_out) |json_out_| {
                            defer json_out_.deinit(allocator);
                            if (comma) {
                                try fout_writer.?.interface.writeAll(",\n");
                            } else {
                                try fout_writer.?.interface.writeByte('\n');
                            }
                            comma = true;
                            try json_writer.write(json_out_);
                        }
                    },
                }
            }
        }

        while (objcache.popRemaining(allocator)) |json_out| {
            defer json_out.deinit(allocator);
            if (comma) {
                try fout_writer.?.interface.writeAll(",\n");
            } else {
                try fout_writer.?.interface.writeByte('\n');
            }
            comma = true;
            try json_writer.write(json_out);
        }
    }
    if (fout_writer) |*f| {
        try f.interface.writeAll("\n]\n");
        try f.interface.flush();
    }
}
