const fs = require('fs');
let code = fs.readFileSync('build.zig', 'utf8');

const search = `    // (like tesseract, which is compiled against libstdc++'s std::vector)
    if (t.os.tag == .linux) {
        exe.linkSystemLibrary("stdc++");
        exe.linkSystemLibrary("unwind");
    } else if (t.os.tag == .windows) {
        // When cross compiling or using target msvc, MSVC uses msvcrt natively. Do not link GNU libc++ here.
        exe.linkLibC();
    } else {
        exe.linkLibCpp();
    }`;

const replace = `    // IMPORTANT: Use system libstdc++ on linux to avoid libc++ ABI mismatch with apt packages
    // (like tesseract, which is compiled against libstdc++'s std::vector)
    if (t.os.tag == .linux) {
        exe.linkSystemLibrary("stdc++");
        exe.linkSystemLibrary("unwind");
    } else if (t.os.tag == .windows) {
        // When targeting msvc, zig needs linkLibCpp to properly pull in MSVC C++ runtime dependencies if building C++ source.
        exe.linkLibCpp();
    } else {
        exe.linkLibCpp();
    }`;

code = code.replace(search, replace);

const search2 = `        // MSVC vcpkg naming drops the standard "lib" prefix
        exe.linkSystemLibrary("tesseract55");
        exe.linkSystemLibrary("leptonica-1.87.0");
        exe.linkSystemLibrary("webp");
        exe.linkSystemLibrary("mecab");`;

const replace2 = `        // When using vcpkg MSVC libs via Zig, the files are typically named .lib but occasionally have different names.
        exe.linkSystemLibrary("tesseract55");
        exe.linkSystemLibrary("leptonica-1.87.0");
        exe.linkSystemLibrary("libwebp");
        exe.linkSystemLibrary("libmecab");`; // We are using zig's linkSystemLibrary, we shouldn't have changed this from libwebp/libmecab based on previous vcpkg logs where it worked. The real issue before was missing windowsapp.lib and not being able to find libcxxabi because it's not MSVC ABI setup.

code = code.replace(search2, replace2);

fs.writeFileSync('build.zig', code);
