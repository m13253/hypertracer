const std = @import("std");

pub const ArgParser = struct {
    arena: *std.heap.ArenaAllocator,
    filename_in: []const []const u8,
    filename_out: ?[]const u8,

    pub const Self = @This();
    pub const Error = error{ArgParseError} || std.mem.Allocator.Error;

    pub fn init(allocator: std.mem.Allocator, args: *std.process.ArgIterator) Error!?Self {
        const arena = try allocator.create(std.heap.ArenaAllocator);
        errdefer allocator.destroy(arena);
        arena.* = std.heap.ArenaAllocator.init(allocator);
        errdefer arena.deinit();
        const arenaAllocator = arena.allocator();

        var program_name: ?[]const u8 = null;
        defer if (program_name) |buf| {
            allocator.free(buf);
        };
        var filename_in = std.ArrayList([]const u8).init(allocator);
        defer filename_in.deinit();
        var filename_out: ?[]const u8 = null;
        var state: enum { Start, Continue, DashDash, DashO } = .Start;
        while (args.next()) |arg| {
            switch (state) {
                .Start => {
                    program_name = try allocator.dupe(u8, arg);
                    state = .Continue;
                },
                .Continue => if (std.mem.eql(u8, arg, "--")) {
                    state = .DashDash;
                } else if (std.mem.eql(u8, arg, "--help") or std.mem.eql(u8, arg, "-?")) {
                    print_help(program_name);
                    arena.deinit();
                    allocator.destroy(arena);
                    return null;
                } else if (std.mem.eql(u8, arg, "-o")) {
                    if (filename_out != null) {
                        std.debug.print("Error: Please specify only one output file.\n", .{});
                        return Error.ArgParseError;
                    }
                    state = .DashO;
                } else if (arg.len > 1 and arg[0] == '-') {
                    std.debug.print("Error: Unknown option: \"{s}\".\n", .{arg});
                    return Error.ArgParseError;
                } else {
                    try filename_in.append(try std.mem.Allocator.dupe(arenaAllocator, u8, arg));
                },
                .DashDash => try filename_in.append(try std.mem.Allocator.dupe(arenaAllocator, u8, arg)),
                .DashO => {
                    state = .Continue;
                    filename_out = try std.mem.Allocator.dupe(arenaAllocator, u8, arg);
                },
            }
        }
        if (state == .DashO) {
            std.debug.print("Error: Option \"-o\" is missing an argument.\n", .{});
            return Error.ArgParseError;
        }
        if (filename_in.items.len == 0) {
            std.debug.print("Error: Please specify input files.\n", .{});
            print_help(program_name);
            return Error.ArgParseError;
        }
        return Self{
            .arena = arena,
            .filename_in = try std.mem.Allocator.dupe(arenaAllocator, []const u8, filename_in.items),
            .filename_out = filename_out,
        };
    }

    pub fn deinit(self: Self) void {
        const child_allocator = self.arena.child_allocator;
        self.arena.deinit();
        child_allocator.destroy(self.arena);
    }

    fn print_help(program_name: ?[]const u8) void {
        std.io.getStdOut().writer().print(
            \\Usage: {s} INPUT.trace [ INPUT.trace ... ] [ -o OUTPUT.json ]
            \\
            \\Arguments:
            \\    --help              Show this help message.
            \\    -o OUTPUT.json      Specify output file name.
            \\
        , .{program_name orelse "decode-trace"}) catch {};
    }
};
