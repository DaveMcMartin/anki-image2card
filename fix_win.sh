sed -i '/exe.linkSystemLibrary("tesseract54");/d' build.zig
sed -i '/exe.linkSystemLibrary("tesseract53");/d' build.zig
sed -i '/exe.linkSystemLibrary("leptonica-1.84.1");/d' build.zig
sed -i '/exe.linkSystemLibrary("leptonica-1.83.0");/d' build.zig
sed -i '/exe.linkSystemLibrary("libwebp");/d' build.zig
sed -i '/exe.linkSystemLibrary("libmecab");/d' build.zig
