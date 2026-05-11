const fs = require('fs');
let code = fs.readFileSync('build.zig', 'utf8');

const search = `    // (like tesseract, which is compiled against libstdc++'s std::vector)
    if (t.os.tag == .linux or t.os.tag == .windows) {
        exe.linkSystemLibrary("stdc++");
        exe.linkSystemLibrary("unwind");
    } else {
        exe.linkLibCpp();
    }`;

const replace = `    // IMPORTANT: Use system libstdc++ on linux to avoid libc++ ABI mismatch with apt packages
    // (like tesseract, which is compiled against libstdc++'s std::vector)
    if (t.os.tag == .linux) {
        exe.linkSystemLibrary("stdc++");
        exe.linkSystemLibrary("unwind");
    } else if (t.os.tag == .windows) {
        exe.linkSystemLibrary("stdc++");
        exe.linkSystemLibrary("unwind");
    } else {
        exe.linkLibCpp();
    }`;

code = code.replace(search, replace);
fs.writeFileSync('build.zig', code);
