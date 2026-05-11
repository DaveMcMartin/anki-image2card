const fs = require('fs');
let code = fs.readFileSync('build.zig', 'utf8');

const search = `    // Tesseract, Leptonica, WebP, MeCab
    if (t.os.tag == .windows) {
        // vcpkg names
        exe.linkSystemLibrary("tesseract55");
        exe.linkSystemLibrary("leptonica-1.87.0");
        exe.linkSystemLibrary("libwebp");
        exe.linkSystemLibrary("libmecab");
    } else {`;

const replace = `    // Tesseract, Leptonica, WebP, MeCab
    if (t.os.tag == .windows) {
        // MSVC linking doesn't prepend "lib" to system libraries typically via vcpkg
        exe.linkSystemLibrary("tesseract55");
        exe.linkSystemLibrary("leptonica-1.87.0");
        exe.linkSystemLibrary("webp");
        exe.linkSystemLibrary("mecab");
    } else {`;

code = code.replace(search, replace);

fs.writeFileSync('build.zig', code);
