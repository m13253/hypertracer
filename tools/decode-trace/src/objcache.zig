const std = @import("std");
const Value = @import("value.zig").Value;

pub const ObjCache = struct {
    allocator: std.mem.Allocator,
    root: std.StringArrayHashMapUnmanaged(RootRecord),
    cache: std.StringHashMapUnmanaged(CacheRecord),

    pub const Self = @This();
    pub const Error = error{
        InvalidTracingFormat,
        PayloadContainerIDConflict,
        PayloadContainerNotFound,
        PayloadContainerTypeMismatch,
    } || std.mem.Allocator.Error;
    const RootRecord = struct {
        value: *Value, // Self-owned
        refcount: usize,
    };
    const CacheRecord = struct {
        value: *Value, // Owned by RootRecord
        root: []const u8, // Owned by RootRecord
    };

    pub fn init(allocator: std.mem.Allocator) Self {
        return Self{
            .allocator = allocator,
            .root = .{},
            .cache = .{},
        };
    }

    pub fn deinit(self: Self) void {
        var cache = self.cache;
        var cache_iter = cache.iterator();
        while (cache_iter.next()) |entry| {
            self.allocator.free(entry.key_ptr.*);
        }
        cache.deinit(self.allocator);
        var root = self.root;
        var root_iter = root.iterator();
        while (root_iter.next()) |entry| {
            self.allocator.free(entry.key_ptr.*);
            entry.value_ptr.value.deinit(self.allocator);
            self.allocator.destroy(entry.value_ptr.value);
        }
        root.deinit(self.allocator);
    }

    pub fn processRecord(self: *Self, record: Value) Error!?Value {
        // [null, [Child]] => Add Child to root
        // [ParentID] => Close the container
        // [ParentID, [Child]] => Add Child to ParentID
        // [ParentID, {Child: Child}] => Add Child: Child to ParentID
        return switch (record) {
            .array => |record_array| switch (record_array.items.len) {
                // A close record
                1 => return self.closeContainer(record_array.items[0].getIdentifier() orelse return Error.InvalidTracingFormat),
                2 => {
                    const parent_id = switch (record_array.items[0].*) {
                        .null => null,
                        else => record_array.items[0].getIdentifier() orelse return Error.InvalidTracingFormat,
                    };
                    try switch (record_array.items[1].*) {
                        .array => |item_array| self.appendToArrayContainer(parent_id, item_array.items),
                        .map => |item_map| self.appendToMapContainer(parent_id, item_map.items),
                        else => Error.InvalidTracingFormat,
                    };
                    return null;
                },
                else => Error.InvalidTracingFormat,
            },
            else => Error.InvalidTracingFormat,
        };
    }

    pub fn popRemaining(self: *Self) ?Value {
        if (self.root.pop()) |entry| {
            self.allocator.free(entry.key);
            const value_copy = entry.value.value.*;
            self.allocator.destroy(entry.value.value);
            return value_copy;
        }
        return null;
    }

    fn closeContainer(self: *Self, container_id: []const u8) Error!?Value {
        const cache_entry = self.cache.fetchRemove(container_id) orelse return Error.PayloadContainerNotFound;
        self.allocator.free(cache_entry.key);
        const root = self.root.getPtr(cache_entry.value.root) orelse return Error.PayloadContainerNotFound;
        root.refcount -= 1;
        if (root.refcount == 0) {
            const root_entry = self.root.fetchOrderedRemove(cache_entry.value.root).?;
            self.allocator.free(root_entry.key);
            const value_copy = root_entry.value.value.*;
            self.allocator.destroy(root_entry.value.value);
            return value_copy;
        }
        return null;
    }

    fn appendToArrayContainer(self: *Self, parent_id: ?[]const u8, item: []const *const Value) Error!void {
        if (parent_id) |parent_id_| {
            const parent = self.cache.get(parent_id_) orelse return Error.PayloadContainerNotFound;
            switch (parent.value.*) {
                .array => |*array| {
                    var child_ids = try std.ArrayList([]const u8).initCapacity(self.allocator, item.len);
                    defer child_ids.deinit();
                    const old_length = array.items.len;
                    errdefer {
                        array.resize(self.allocator, old_length) catch {};
                        for (child_ids.items) |child_id| {
                            if (self.closeContainer(child_id) catch null) |child_value| {
                                child_value.deinit(self.allocator);
                            }
                        }
                    }
                    for (item) |i| {
                        const child = try self.processChild(parent_id, i.*);
                        if (child[0]) |child_id| {
                            try child_ids.append(child_id);
                        }
                        try array.append(self.allocator, child[1]);
                    }
                },
                else => return Error.PayloadContainerTypeMismatch,
            }
        } else {
            var child_ids = try std.ArrayList([]const u8).initCapacity(self.allocator, item.len);
            defer child_ids.deinit();
            errdefer {
                for (child_ids.items) |child_id| {
                    if (self.closeContainer(child_id) catch null) |child_value| {
                        child_value.deinit(self.allocator);
                    }
                }
            }
            for (item) |i| {
                const child = try self.processChild(parent_id, i.*);
                if (child[0]) |child_id| {
                    try child_ids.append(child_id);
                }
            }
        }
    }

    fn appendToMapContainer(self: *Self, parent_id: ?[]const u8, item: []const Value.MapStruct) Error!void {
        if (parent_id == null) {
            return Error.PayloadContainerTypeMismatch;
        }
        const parent = self.cache.get(parent_id.?) orelse return Error.PayloadContainerNotFound;
        switch (parent.value.*) {
            .map => |*map| {
                var child_ids = try std.ArrayList([]const u8).initCapacity(self.allocator, item.len);
                defer child_ids.deinit();
                const old_length = map.items.len;
                errdefer {
                    map.resize(self.allocator, old_length) catch {};
                    for (child_ids.items) |child_id| {
                        if (self.closeContainer(child_id) catch null) |child_value| {
                            child_value.deinit(self.allocator);
                        }
                    }
                }
                for (item) |i| {
                    const child_key = try self.processChild(parent_id, i.key.*);
                    if (child_key[0]) |child_id| {
                        try child_ids.append(child_id);
                    }
                    const child_value = try self.processChild(parent_id, i.value.*);
                    if (child_value[0]) |child_id| {
                        try child_ids.append(child_id);
                    }
                    try map.append(self.allocator, Value.MapStruct{ .key = child_key[1], .value = child_value[1] });
                }
            },
            else => return Error.PayloadContainerTypeMismatch,
        }
    }

    // Return value: { child_id, child_value }
    // child_id has the same lifetime as item,
    // child_value is owned by RootRecord.
    fn processChild(self: *Self, parent_id: ?[]const u8, item: Value) Error!struct { ?[]const u8, *Value } {
        // [ID, []] => Open a new array
        // [ID, {}] => Open a new map
        const child_id: ?[]const u8, const child = blk1: switch (item) {
            .array => |array| {
                if (array.items.len != 2) {
                    return Error.InvalidTracingFormat;
                }
                break :blk1 .{
                    array.items[0].getIdentifier() orelse return Error.InvalidTracingFormat,
                    blk2: switch (array.items[1].*) {
                        .array => |child_array| {
                            if (child_array.items.len != 0) {
                                return Error.InvalidTracingFormat;
                            }
                            const child_value = try self.allocator.create(Value);
                            errdefer self.allocator.destroy(child_value);
                            child_value.* = Value{ .array = std.ArrayListUnmanaged(*Value){} };
                            break :blk2 child_value;
                        },
                        .map => |child_map| {
                            if (child_map.items.len != 0) {
                                return Error.InvalidTracingFormat;
                            }
                            const child_value = try self.allocator.create(Value);
                            errdefer self.allocator.destroy(child_value);
                            child_value.* = Value{ .map = std.ArrayListUnmanaged(Value.MapStruct){} };
                            break :blk2 child_value;
                        },
                        else => return Error.InvalidTracingFormat,
                    },
                };
            },
            .stream_array_start, .map, .break_mark => return Error.InvalidTracingFormat,
            else => {
                const child_value = try self.allocator.create(Value);
                errdefer self.allocator.destroy(child_value);
                child_value.* = try item.clone(self.allocator);
                break :blk1 .{ null, child_value };
            },
        };
        if (child_id) |child_id_| {
            const root_id_ptr = blk: {
                const container_id_key = try self.allocator.dupe(u8, child_id_);
                errdefer self.allocator.free(container_id_key);
                const cache_entry = try self.cache.getOrPut(self.allocator, container_id_key);
                if (cache_entry.found_existing) {
                    return Error.PayloadContainerIDConflict;
                }
                cache_entry.value_ptr.* = CacheRecord{ .value = child, .root = undefined };
                break :blk &cache_entry.value_ptr.root;
            };
            errdefer if (self.cache.fetchRemove(child_id_)) |entry| {
                self.allocator.free(entry.key);
                entry.value.value.deinit(self.allocator);
                self.allocator.destroy(entry.value.value);
            };
            if (parent_id) |parent_id_| {
                const root_id = (self.cache.get(parent_id_) orelse return Error.PayloadContainerNotFound).root;
                const root_entry = self.root.getEntry(root_id) orelse return Error.PayloadContainerNotFound;
                root_id_ptr.* = root_entry.key_ptr.*;
                root_entry.value_ptr.refcount += 1;
            } else {
                const root_id_key = try self.allocator.dupe(u8, child_id_);
                errdefer self.allocator.free(root_id_key);
                const root_entry = try self.root.getOrPut(self.allocator, root_id_key);
                if (root_entry.found_existing) {
                    return Error.PayloadContainerIDConflict;
                }
                root_entry.value_ptr.* = RootRecord{ .value = child, .refcount = 1 };
                root_id_ptr.* = root_entry.key_ptr.*;
            }
        }
        return .{ child_id, child };
    }
};
