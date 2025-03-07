const std = @import("std");
const Value = @import("value.zig").Value;

pub fn Parser(comptime ReaderType: type) type {
    return struct {
        allocator: std.mem.Allocator,
        r: std.io.CountingReader(ReaderType),

        pub const Self = @This();
        pub const Error = error{
            UnexpectedEndOfInput,
            UnsupportedDataType,
        } || std.mem.Allocator.Error || std.io.CountingReader(ReaderType).Reader.NoEofError;

        pub fn init(allocator: std.mem.Allocator, r: ReaderType) Self {
            return Self{
                .allocator = allocator,
                .r = std.io.countingReader(r),
            };
        }

        pub fn bytesRead(self: Self) u64 {
            return self.r.bytes_read;
        }

        pub fn nextValue(self: *Self) Error!?Value {
            const b = self.r.reader().readByte() catch |err| {
                if (err == Error.EndOfStream) {
                    return null;
                }
                return err;
            };
            return switch (b & 0xe0) {
                0x00 => Value{ .pos_int = try self.nextInt(b) },
                0x20 => Value{ .neg_int = try self.nextInt(b) },
                0x40 => blk: {
                    const size = try self.nextInt(b);
                    const bytes = try self.allocator.alloc(u8, size);
                    errdefer self.allocator.free(bytes);
                    try self.r.reader().readNoEof(bytes);
                    break :blk Value{ .bytes = bytes };
                },
                0x60 => blk: {
                    const size = try self.nextInt(b);
                    const string = try self.allocator.alloc(u8, size);
                    errdefer self.allocator.free(string);
                    try self.r.reader().readNoEof(string);
                    break :blk Value{ .string = string };
                },
                0x80 => if (b != 0x9f) blk: {
                    const size = try self.nextInt(b);
                    var array = try std.ArrayListUnmanaged(*Value).initCapacity(self.allocator, size);
                    errdefer {
                        for (array.items) |item| {
                            item.deinit(self.allocator);
                            self.allocator.destroy(item);
                        }
                        array.deinit(self.allocator);
                    }
                    for (0..size) |_| {
                        const item = try self.allocator.create(Value);
                        errdefer self.allocator.destroy(item);
                        item.* = (try self.nextValue()) orelse return Error.EndOfStream;
                        errdefer item.deinit(self.allocator);
                        try array.append(self.allocator, item);
                    }
                    break :blk Value{ .array = array };
                } else blk: {
                    break :blk Value{ .stream_array_start = {} };
                },
                0xa0 => blk: {
                    const size = try self.nextInt(b);
                    var map = try std.ArrayListUnmanaged(Value.MapStruct).initCapacity(self.allocator, size);
                    errdefer {
                        for (map.items) |item| {
                            item.value.deinit(self.allocator);
                            self.allocator.destroy(item.value);
                            item.key.deinit(self.allocator);
                            self.allocator.destroy(item.key);
                        }
                        map.deinit(self.allocator);
                    }
                    for (0..size) |_| {
                        const key = try self.allocator.create(Value);
                        errdefer self.allocator.destroy(key);
                        key.* = (try self.nextValue()) orelse return Error.EndOfStream;
                        errdefer key.deinit(self.allocator);
                        const value = try self.allocator.create(Value);
                        errdefer self.allocator.destroy(value);
                        value.* = (try self.nextValue()) orelse return Error.EndOfStream;
                        errdefer value.deinit(self.allocator);
                        try map.append(self.allocator, Value.MapStruct{ .key = key, .value = value });
                    }
                    break :blk Value{ .map = map };
                },
                0xc0 => blk: {
                    const tag = try self.nextInt(b);
                    const value = try self.allocator.create(Value);
                    errdefer self.allocator.destroy(value);
                    value.* = (try self.nextValue()) orelse return Error.EndOfStream;
                    break :blk Value{ .tag = Value.TagStruct{ .tag = tag, .value = value } };
                },
                0xe0 => switch (b) {
                    0xf4 => Value{ .false = {} },
                    0xf5 => Value{ .true = {} },
                    0xf6 => Value{ .null = {} },
                    0xfa => Value{ .float32 = @bitCast(std.mem.readInt(u32, &try self.r.reader().readBytesNoEof(4), .big)) },
                    0xfb => Value{ .float64 = @bitCast(std.mem.readInt(u64, &try self.r.reader().readBytesNoEof(8), .big)) },
                    0xff => Value{ .break_mark = {} },
                    else => Error.UnsupportedDataType,
                },
                else => unreachable,
            };
        }

        fn nextInt(self: *Self, prefix: u8) Error!u64 {
            const b = prefix & 0x1f;
            return switch (b) {
                0x00...0x17 => b,
                0x18 => try self.r.reader().readByte(),
                0x19 => std.mem.readInt(u16, &try self.r.reader().readBytesNoEof(2), .big),
                0x1a => std.mem.readInt(u32, &try self.r.reader().readBytesNoEof(4), .big),
                0x1b => std.mem.readInt(u64, &try self.r.reader().readBytesNoEof(8), .big),
                0x1c...0x1f => return Error.UnsupportedDataType,
                else => unreachable,
            };
        }
    };
}
