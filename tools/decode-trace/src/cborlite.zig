const std = @import("std");
const Value = @import("value.zig").Value;

pub fn Parser(comptime ReaderType: type) type {
    return struct {
        allocator: std.mem.Allocator,
        r: std.io.CountingReader(ReaderType),

        const Self = @This();
        const Error = error{
            UnexpectedEndOfInput,
            UnsupportedDataType,
        } || std.mem.Allocator.Error || std.io.CountingReader(ReaderType).Error || std.io.CountingReader(ReaderType).Reader.NoEofError;

        pub fn init(allocator: std.mem.Allocator, r: ReaderType) Self {
            return Self{
                .allocator = allocator,
                .r = std.io.countingReader(r),
            };
        }

        pub fn bytesRead(self: Self) u64 {
            return self.r.bytes_read;
        }

        pub fn deinitValue(self: Self, value: Value) void {
            switch (value) {
                .pos_int, .neg_int => {},
                .bytes => |item| self.allocator.free(item),
                .string => |item| self.allocator.free(item),
                .array => |array| for (array) |item| {
                    self.deinitValue(item);
                },
                .stream_array_start => {},
                .map => |map| for (map) |item| {
                    self.deinitValue(item.key);
                    self.deinitValue(item.value);
                },
                .tag => |tag| {
                    self.deinitValue(tag.value);
                    self.allocator.free(tag);
                },
                .false, .true, .null, .float32, .float64, .break_mark => {},
            }
        }

        pub fn nextValue(self: *Self) Error!Value {
            const b = try self.r.reader().readByte();
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
                0x80 => if (b != 0x8f) blk: {
                    const size = try self.nextInt(b);
                    const array = try self.allocator.alloc(Value, size);
                    errdefer self.allocator.free(array);
                    for (array) |*item| {
                        item.* = try self.nextValue();
                    }
                    break :blk Value{ .array = array };
                } else blk: {
                    break :blk Value{ .stream_array_start = {} };
                },
                0xa0 => blk: {
                    const size = try self.nextInt(b);
                    const map = try self.allocator.alloc(Value.MapStruct, size);
                    errdefer self.allocator.free(map);
                    for (map) |*item| {
                        item.key = try self.nextValue();
                        item.value = try self.nextValue();
                    }
                    break :blk Value{ .map = map };
                },
                0xc0 => blk: {
                    const tag = try self.allocator.create(Value.TagStruct);
                    errdefer self.allocator.destroy(tag);
                    tag.tag = try self.nextInt(b);
                    tag.value = try self.nextValue();
                    break :blk Value{ .tag = tag };
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
