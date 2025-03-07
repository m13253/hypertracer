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
    array: std.ArrayListUnmanaged(*Value),
    stream_array_start,
    map: std.ArrayListUnmanaged(MapStruct),
    tag: TagStruct,
    false,
    true,
    null,
    float32: f32,
    float64: f64,
    break_mark,

    pub const Self = @This();
    pub const MapStruct = struct { key: *Value, value: *Value };
    pub const TagStruct = struct { tag: u64, value: *Value };

    pub fn deinit(self: Self, allocator: std.mem.Allocator) void {
        switch (self) {
            .pos_int, .neg_int => {},
            .bytes => |item| allocator.free(item),
            .string => |item| allocator.free(item),
            .array => |array_| {
                var array = array_;
                for (array.items) |item| {
                    item.deinit(allocator);
                    allocator.destroy(item);
                }
                array.deinit(allocator);
            },
            .stream_array_start => {},
            .map => |map_| {
                var map = map_;
                for (map.items) |item| {
                    item.value.deinit(allocator);
                    allocator.destroy(item.value);
                    item.key.deinit(allocator);
                    allocator.destroy(item.key);
                }
                map.deinit(allocator);
            },
            .tag => |tag| {
                tag.value.deinit(allocator);
                allocator.destroy(tag.value);
            },
            .false, .true, .null, .float32, .float64, .break_mark => {},
        }
    }

    pub fn clone(self: Self, allocator: std.mem.Allocator) std.mem.Allocator.Error!Self {
        return switch (self) {
            .pos_int, .neg_int => self,
            .bytes => |item| Self{ .bytes = try allocator.dupe(u8, item) },
            .string => |item| Self{ .string = try allocator.dupe(u8, item) },
            .array => |old_array| blk: {
                var array = try std.ArrayListUnmanaged(*Self).initCapacity(allocator, old_array.items.len);
                errdefer {
                    for (array.items) |item| {
                        item.deinit(allocator);
                        allocator.destroy(item);
                    }
                    array.deinit(allocator);
                }
                for (old_array.items) |old_item| {
                    const item = try allocator.create(Self);
                    errdefer allocator.destroy(item);
                    item.* = try old_item.clone(allocator);
                    errdefer item.deinit(allocator);
                    try array.append(allocator, item);
                }
                break :blk Self{ .array = array };
            },
            .stream_array_start => self,
            .map => |old_map| blk: {
                var map = try std.ArrayListUnmanaged(MapStruct).initCapacity(allocator, old_map.items.len);
                errdefer {
                    for (map.items) |item| {
                        item.value.deinit(allocator);
                        allocator.destroy(item.value);
                        item.key.deinit(allocator);
                        allocator.destroy(item.key);
                    }
                    map.deinit(allocator);
                }
                for (old_map.items) |old_item| {
                    const key = try allocator.create(Self);
                    errdefer allocator.destroy(key);
                    key.* = try old_item.key.clone(allocator);
                    errdefer key.deinit(allocator);
                    const value = try allocator.create(Self);
                    errdefer allocator.destroy(value);
                    value.* = try old_item.value.clone(allocator);
                    errdefer value.deinit(allocator);
                    try map.append(allocator, MapStruct{ .key = key, .value = value });
                }
                break :blk Self{ .map = map };
            },
            .tag => |old_tag| blk: {
                const value = try allocator.create(Self);
                errdefer allocator.destroy(value);
                value.* = try old_tag.value.clone(allocator);
                break :blk Self{ .tag = TagStruct{ .tag = old_tag.tag, .value = value } };
            },
            .false, .true, .null, .float32, .float64, .break_mark => self,
        };
    }

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
                39 => switch (tag.value.*) {
                    .bytes => |id| id,
                    else => null,
                },
                else => null,
            },
            else => null,
        };
    }
};
