const std = @import("std");
const Value = @import("value.zig").Value;

pub const Parser = struct {
    r: *std.Io.Reader,
    bytes_read: u64,

    pub const Self = @This();
    pub const Error = error{
        UnsupportedDataType,
    } || std.mem.Allocator.Error || std.Io.Reader.Error;

    pub fn init(r: *std.Io.Reader) Self {
        return Self{
            .r = r,
            .bytes_read = 0,
        };
    }

    pub fn bytesRead(self: Self) u64 {
        return self.bytes_read;
    }

    pub fn nextValue(self: *Self, allocator: std.mem.Allocator) Error!?Value {
        const b = self.r.takeByte() catch |err| {
            return if (err == Error.EndOfStream)
                null
            else
                err;
        };
        self.bytes_read += @sizeOf(u8);
        return switch (b & 0xe0) {
            0x00 => Value{ .pos_int = try self.nextInt(b) },
            0x20 => Value{ .neg_int = try self.nextInt(b) },
            0x40 => {
                const size = try self.nextInt(b);
                const bytes = try allocator.alloc(u8, size);
                errdefer allocator.free(bytes);
                try self.r.readSliceAll(bytes);
                self.bytes_read += size;
                return Value{ .bytes = bytes };
            },
            0x60 => {
                const size = try self.nextInt(b);
                const string = try allocator.alloc(u8, size);
                errdefer allocator.free(string);
                try self.r.readSliceAll(string);
                self.bytes_read += size;
                return Value{ .string = string };
            },
            0x80 => if (b != 0x9f) {
                const size = try self.nextInt(b);
                var array = try std.ArrayListUnmanaged(*Value).initCapacity(allocator, size);
                errdefer {
                    for (array.items) |item| {
                        item.deinit(allocator);
                        allocator.destroy(item);
                    }
                    array.deinit(allocator);
                }
                for (0..size) |_| {
                    const item = try allocator.create(Value);
                    errdefer allocator.destroy(item);
                    item.* = (try self.nextValue(allocator)) orelse return Error.EndOfStream;
                    errdefer item.deinit(allocator);
                    try array.append(allocator, item);
                }
                return Value{ .array = array };
            } else {
                return Value{ .stream_array_start = {} };
            },
            0xa0 => {
                const size = try self.nextInt(b);
                var map = try std.ArrayListUnmanaged(Value.MapStruct).initCapacity(allocator, size);
                errdefer {
                    for (map.items) |item| {
                        item.value.deinit(allocator);
                        allocator.destroy(item.value);
                        item.key.deinit(allocator);
                        allocator.destroy(item.key);
                    }
                    map.deinit(allocator);
                }
                for (0..size) |_| {
                    const key = try allocator.create(Value);
                    errdefer allocator.destroy(key);
                    key.* = (try self.nextValue(allocator)) orelse return Error.EndOfStream;
                    errdefer key.deinit(allocator);
                    const value = try allocator.create(Value);
                    errdefer allocator.destroy(value);
                    value.* = (try self.nextValue(allocator)) orelse return Error.EndOfStream;
                    errdefer value.deinit(allocator);
                    try map.append(allocator, Value.MapStruct{ .key = key, .value = value });
                }
                return Value{ .map = map };
            },
            0xc0 => {
                const tag = try self.nextInt(b);
                const value = try allocator.create(Value);
                errdefer allocator.destroy(value);
                value.* = (try self.nextValue(allocator)) orelse return Error.EndOfStream;
                return Value{ .tag = Value.TagStruct{ .tag = tag, .value = value } };
            },
            0xe0 => switch (b) {
                0xf4 => Value{ .false = {} },
                0xf5 => Value{ .true = {} },
                0xf6 => Value{ .null = {} },
                0xfa => {
                    const number: f32 = @bitCast(try self.r.takeInt(u32, .big));
                    self.bytes_read += @sizeOf(u32);
                    return Value{ .float32 = number };
                },
                0xfb => {
                    const number: f64 = @bitCast(try self.r.takeInt(u64, .big));
                    self.bytes_read += @sizeOf(u64);
                    return Value{ .float64 = number };
                },
                0xff => Value{ .break_mark = {} },
                else => Error.UnsupportedDataType,
            },
            else => unreachable,
        };
    }

    fn nextInt(self: *Self, prefix: u8) Error!u64 {
        const b = prefix & 0x1f;
        switch (b) {
            0x00...0x17 => return b,
            0x18 => {
                const number = try self.r.takeByte();
                self.bytes_read += @sizeOf(u8);
                return number;
            },
            0x19 => {
                const number = try self.r.takeInt(u16, .big);
                self.bytes_read += @sizeOf(u16);
                return number;
            },
            0x1a => {
                const number = try self.r.takeInt(u32, .big);
                self.bytes_read += @sizeOf(u32);
                return number;
            },
            0x1b => {
                const number = try self.r.takeInt(u64, .big);
                self.bytes_read += @sizeOf(u64);
                return number;
            },
            0x1c...0x1f => return Error.UnsupportedDataType,
            else => unreachable,
        }
    }
};
