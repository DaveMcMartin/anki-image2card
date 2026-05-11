const fs = require('fs');
let code = fs.readFileSync('build.zig', 'utf8');

const search = `    // Tesseract, Leptonica, WebP, MeCab
    if (t.os.tag == .windows) {
        exe.linkSystemLibrary("tesseract");
        exe.linkSystemLibrary("lept");
        exe.linkSystemLibrary("webp");
        exe.linkSystemLibrary("mecab");
    } else {
        exe.linkSystemLibrary("tesseract");
        exe.linkSystemLibrary("lept");
        exe.linkSystemLibrary("webp");
        exe.linkSystemLibrary("mecab");
    }`;

const replace = `    // Tesseract, Leptonica, WebP, MeCab
    if (t.os.tag == .windows) {
        exe.linkSystemLibrary("tesseract55");
        exe.linkSystemLibrary("leptonica-1.87.0");
        exe.linkSystemLibrary("libwebp");
        exe.linkSystemLibrary("libmecab");
    } else {
        exe.linkSystemLibrary("tesseract");
        exe.linkSystemLibrary("lept");
        exe.linkSystemLibrary("webp");
        exe.linkSystemLibrary("mecab");
    }`;

code = code.replace(search, replace);

fs.writeFileSync('build.zig', code);
