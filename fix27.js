const fs = require('fs');
let code = fs.readFileSync('build.zig', 'utf8');

const search = `    if (t.os.tag == .windows) {
        // vcpkg names
        exe.linkSystemLibrary("tesseract55");
        exe.linkSystemLibrary("leptonica-1.87.0");
        exe.linkSystemLibrary("webp");
        exe.linkSystemLibrary("mecab");
    } else {`;

const replace = `    if (t.os.tag == .windows) {
        // vcpkg names
        exe.linkSystemLibrary("tesseract55");
        exe.linkSystemLibrary("leptonica-1.87.0");
        exe.linkSystemLibrary("libwebp");
        exe.linkSystemLibrary("libmecab");
    } else {`;

code = code.replace(search, replace);

fs.writeFileSync('build.zig', code);
