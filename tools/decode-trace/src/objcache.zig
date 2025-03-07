const std = @import("std");
const Value = @import("value.zig").Value;

pub const ObjCacheStruct = struct {
    allocator: std.mem.Allocator,
    root: std.StringHashMapUnmanaged(Value),
    cache: std.StringHashMapUnmanaged(*Value),

    pub const Self = @This();
    const RootRecord = struct {
        value: Value,
        refcount: usize = 0,
    };
    const CacheRecord = struct {
        value: *Value,
        root: *RootRecord,
    };

    pub fn init(allocator: std.mem.Allocator) Self {
        return Self{
            .allocator = allocator,
            .root = .{},
            .cache = .{},
        };
    }

    pub fn deinit(self: Self) void {
        self.cache.deinit(self.allocator);
        self.root.deinit(self.allocator);
    }
};
