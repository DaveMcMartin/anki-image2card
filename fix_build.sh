#!/bin/bash
git checkout build.zig
sed -i 's/if (t.os.tag == .windows) {//g' build.zig
sed -i 's/exe.linkSystemLibrary("tesseract54");//g' build.zig
sed -i 's/exe.linkSystemLibrary("tesseract53");//g' build.zig
sed -i 's/exe.linkSystemLibrary("leptonica-1.84.1");//g' build.zig
sed -i 's/exe.linkSystemLibrary("leptonica-1.83.0");//g' build.zig
sed -i 's/exe.linkSystemLibrary("libwebp");//g' build.zig
sed -i 's/exe.linkSystemLibrary("libmecab");//g' build.zig
sed -i 's/} else {//g' build.zig
