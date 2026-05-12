const fs = require('fs');
let code = fs.readFileSync('build.zig', 'utf8');

const search = `    // IMPORTANT: Use system libstdc++ on linux to avoid libc++ ABI mismatch with apt packages
    // (like tesseract, which is compiled against libstdc++'s std::vector)
    if (t.os.tag == .linux) {
        exe.linkLibC();
        exe.linkSystemLibrary("stdc++");
        exe.linkSystemLibrary("unwind");
    } else if (t.os.tag == .windows) {
        exe.linkLibC();
    } else {
        exe.linkLibC();
        exe.linkLibCpp();
    }`;

const replace = `    // IMPORTANT: Use system libstdc++ on linux to avoid libc++ ABI mismatch with apt packages
    // (like tesseract, which is compiled against libstdc++'s std::vector)
    if (t.os.tag == .linux) {
        exe.linkLibC();
        exe.linkSystemLibrary("stdc++");
        exe.linkSystemLibrary("unwind");
    } else if (t.os.tag == .windows) {
        exe.linkLibC();
        exe.linkLibCpp();
    } else {
        exe.linkLibC();
        exe.linkLibCpp();
    }`;

code = code.replace(search, replace);

fs.writeFileSync('build.zig', code);
