const std = @import("std");

pub const ArgParser = struct {
    arena: *std.heap.ArenaAllocator,
    filename_in: std.ArrayList([]const u8),
    filename_out: ?[]const u8 = null,
    will_print_help: bool = false,

    const Self = @This();
    const Error = error{ArgParseError} || std.mem.Allocator.Error;

    pub fn init(allocator: std.mem.Allocator, args: *std.process.ArgIterator) Error!Self {
        const arena = try allocator.create(std.heap.ArenaAllocator);
        errdefer allocator.destroy(arena);
        arena.* = std.heap.ArenaAllocator.init(allocator);
        errdefer arena.deinit();
        const arenaAllocator = arena.allocator();

        var parsed = Self{
            .arena = arena,
            .filename_in = std.ArrayList([]const u8).init(allocator),
        };
        errdefer parsed.filename_in.deinit();

        var state: enum { Start, DashDash, DashO } = .Start;
        while (args.next()) |arg| {
            switch (state) {
                .Start => if (std.mem.eql(u8, arg, "--")) {
                    state = .DashDash;
                } else if (std.mem.eql(u8, arg, "--help") or std.mem.eql(u8, arg, "-?")) {
                    parsed.will_print_help = true;
                } else if (std.mem.eql(u8, arg, "-o")) {
                    if (parsed.filename_out != null) {
                        std.debug.print("Error: Only one \"-o\" is allowed.\n", .{});
                        return Error.ArgParseError;
                    }
                    state = .DashO;
                } else if (arg.len > 1 and arg[0] == '-') {
                    std.debug.print("Error: Unknown option: \"{s}\"\n", .{arg});
                    return Error.ArgParseError;
                } else {
                    try parsed.filename_in.append(try std.mem.Allocator.dupe(arenaAllocator, u8, arg));
                },
                .DashDash => try parsed.filename_in.append(try std.mem.Allocator.dupe(arenaAllocator, u8, arg)),
                .DashO => {
                    state = .Start;
                    parsed.filename_out = try std.mem.Allocator.dupe(arenaAllocator, u8, arg);
                },
            }
        }
        if (state == .DashO) {
            std.debug.print("Error: Option \"-o\" is missing an argument.\n", .{});
            return Error.ArgParseError;
        }
        return parsed;
    }

    pub fn deinit(self: Self) void {
        self.filename_in.deinit();
        const child_allocator = self.arena.child_allocator;
        self.arena.deinit();
        child_allocator.destroy(self.arena);
    }
};
