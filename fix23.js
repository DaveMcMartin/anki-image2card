const fs = require('fs');
let code = fs.readFileSync('build.zig', 'utf8');

const search = `    } else if (t.os.tag == .windows) {
        // vcpkg names
        exe.linkSystemLibrary("tesseract55");
        exe.linkSystemLibrary("leptonica-1.87.0");
        exe.linkSystemLibrary("libwebp");
        exe.linkSystemLibrary("mecab");`;

const replace = `    } else if (t.os.tag == .windows) {
        // MSVC vcpkg naming drops the standard "lib" prefix
        exe.linkSystemLibrary("tesseract55");
        exe.linkSystemLibrary("leptonica-1.87.0");
        exe.linkSystemLibrary("webp");
        exe.linkSystemLibrary("mecab");`;

code = code.replace(search, replace);

fs.writeFileSync('build.zig', code);
