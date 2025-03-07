.PHONY: all clean

RM=rm
ZIG=zig
ZIGFLAGS=

all: zig-out/bin/decode-trace

clean:
	$(RM) -rf .zig-cache zig-out

zig-out/bin/decode-trace: src/argparser.zig src/cborlite.zig src/converter.zig src/jsonlite.zig src/main.zig src/objcache.zig src/value.zig
	$(ZIG) build --release=fast --summary all $(ZIGFLAGS)
