const fs = require('fs');
let code = fs.readFileSync('build.zig', 'utf8');

const search = `    } else if (t.os.tag == .windows) {
        // use libc++ on Windows via Zig
        exe.linkLibCpp();
    } else {
        exe.linkLibCpp();
    }`;

const replace = `    } else if (t.os.tag == .windows) {
        // When cross compiling or using target msvc, MSVC uses msvcrt natively. Do not link GNU libc++ here.
        exe.linkLibC();
    } else {
        exe.linkLibCpp();
    }`;

code = code.replace(search, replace);

fs.writeFileSync('build.zig', code);
