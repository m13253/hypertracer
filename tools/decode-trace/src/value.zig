const std = @import("std");

pub const Tag = enum(u8) {
    pos_int = 0x00,
    neg_int = 0x20,
    bytes = 0x40,
    string = 0x60,
    array = 0x80,
    stream_array_start = 0x9f,
    map = 0xa0,
    tag = 0xc0,
    false = 0xf4,
    true = 0xf5,
    null = 0xf6,
    float32 = 0xfa,
    float64 = 0xfb,
    break_mark = 0xff,
};

pub const Value = union(Tag) {
    pos_int: u64,
    neg_int: u64,
    bytes: []u8,
    string: []u8,
    array: []Value,
    stream_array_start,
    map: []MapStruct,
    tag: *TagStruct,
    false,
    true,
    null,
    float32: f32,
    float64: f64,
    break_mark,

    pub const MapStruct = struct { key: Value, value: Value };
    pub const TagStruct = struct { tag: u64, value: Value };

    pub fn getInt65(self: Value) ?i65 {
        return switch (self) {
            .pos_int => |item| item,
            .neg_int => |item| @as(i65, -1) - item,
            .float32, .float64 => |item| if (@trunc(item) == item and item <= std.math.maxInt(i65) and item >= std.math.minInt(i65))
                @intFromFloat(item)
            else
                null,
            else => null,
        };
    }

    pub fn getIdentifier(self: Value) ?[]const u8 {
        return switch (self) {
            .tag => |tag| switch (tag.tag) {
                39 => switch (tag.value) {
                    .bytes => |id| id,
                    else => null,
                },
                else => null,
            },
            else => null,
        };
    }
};
