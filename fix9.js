const fs = require('fs');
let code = fs.readFileSync('build.zig', 'utf8');

const search = `    // Tesseract, Leptonica, WebP, MeCab
    if (t.os.tag == .windows) {
        // vcpkg names
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

const replace = `    // Tesseract, Leptonica, WebP, MeCab
    if (t.os.tag == .windows) {
        // vcpkg names
        exe.linkSystemLibrary("tesseract55");
        exe.linkSystemLibrary("leptonica-1.87.0");
        exe.linkSystemLibrary("libwebp");
        exe.linkSystemLibrary("mecab");
    } else {
        exe.linkSystemLibrary("tesseract");
        exe.linkSystemLibrary("lept");
        exe.linkSystemLibrary("webp");
        exe.linkSystemLibrary("mecab");
    }`;

const search2 = `        exe.linkSystemLibrary("shlwapi");
        exe.linkSystemLibrary("windowsapp");
    }`;

const replace2 = `        exe.linkSystemLibrary("shlwapi");
        exe.linkSystemLibrary("ole32");
        exe.linkSystemLibrary("oleaut32");
        exe.linkSystemLibrary("mfuuid");
        exe.linkSystemLibrary("mfplat");
        exe.linkSystemLibrary("mfreadwrite");
        exe.linkSystemLibrary("propsys");
    }`;

code = code.replace(search, replace).replace(search2, replace2);
fs.writeFileSync('build.zig', code);
