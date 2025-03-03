const std = @import("std");
const Value = @import("value.zig").Value;

pub const ObjCacheStruct = struct {
    root: std.StringHashMap(void),
    cache: std.StringHashMap(Value),
};
