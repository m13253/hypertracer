const std = @import("std");
const Value = @import("value.zig").Value;

pub fn Writer(comptime WriterType: type) type {
    return struct {
        w: WriterType,

        const Self = @This();
        const Error = error{
            UnsupportedDataType,
            UnsupportedTimestampFormat,
        } || WriterType.Error;

        pub fn init(w: WriterType) Self {
            return Self{
                .w = w,
            };
        }

        pub fn write(self: *Self, value: Value) Error!void {
            return switch (value) {
                .pos_int => |item| std.fmt.formatInt(item, 10, .lower, .{}, self.w),
                .neg_int => |item| if (item == 0xffff_ffff_ffff_ffff)
                    self.w.witeAll("-18446744073709551616")
                else
                    self.w.print("-{}", .{item + 1}),
                .bytes => Error.UnsupportedDataType,
                .string => |item| std.json.encodeJsonString(item, .{}, self.w),
                .array => |array| {
                    try self.w.writeByte('[');
                    var comma = false;
                    for (array) |item| {
                        if (comma)
                            try self.w.writeByte(',');
                        comma = true;
                        try self.write(item);
                    }
                    try self.w.writeByte(']');
                },
                .stream_array_start => Error.UnsupportedDataType,
                .map => |map| {
                    try self.w.writeByte('{');
                    var comma = false;
                    for (map) |item| {
                        if (comma)
                            try self.w.writeByte(',');
                        comma = true;
                        try self.write(item.key);
                        try self.w.writeByte(':');
                        try self.write(item.value);
                    }
                    try self.w.writeByte('}');
                },
                .tag => |tag| switch (tag.tag) {
                    1001 => switch (tag.value) {
                        .map => |timestamp| {
                            var sec: ?i65 = null;
                            var nsec: ?i65 = null;
                            for (timestamp) |component| {
                                switch (component.key.toInt65()) {
                                    1 => {
                                        if (sec == null)
                                            return Error.UnsupportedTimestampFormat;
                                        sec = component.value.toInt65();
                                    },
                                    -9 => {
                                        if (nsec == null)
                                            return Error.UnsupportedTimestampFormat;
                                        nsec = component.value.toInt65();
                                    },
                                    else => return Error.UnsupportedTimestampFormat,
                                }
                            }
                            if (sec == null or nsec == null)
                                return Error.UnsupportedTimestampFormat;
                            try self.printTimestamp(sec.?, nsec.?);
                        },
                        else => Error.UnsupportedDataType,
                    },
                    else => Error.UnsupportedDataType,
                },
                .false => self.w.writeAll("false"),
                .true => self.w.writeAll("true"),
                .null => self.w.writeAll("null"),
                .float32 => |item| self.w.print("{}", item),
                .float64 => |item| self.w.print("{}", item),
                .break_mark => Error.UnsupportedDataType,
            };
        }

        fn printTimestamp(self: *Self, sec: i65, nsec: i65) WriterType.Error!void {
            const total = @as(i128, sec) * 1_000_000_000 + nsec;
            var msec = @divFloor(total, 1000);
            var frac: u32 = @intCast(@mod(total, 1000));
            if (msec < 0 and frac != 0) {
                msec += 1;
                frac = 1000 - frac;
            }
            return self.w.print("{}.{:0>3}", .{ msec, frac });
        }
    };
}
