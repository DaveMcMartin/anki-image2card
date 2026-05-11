const fs = require('fs');
let code = fs.readFileSync('build.zig', 'utf8');

const search = `    } else if (t.os.tag == .windows) {
        // use libc++ on Windows via Zig? The error says: "clang frontend command failed with exit code 139".
        // Let's not use linkLibC nor linkLibCpp, since msvc might do it automatically.
        exe.linkLibC();
        exe.linkLibCpp();
    } else {`;

const replace = `    } else if (t.os.tag == .windows) {
        exe.linkLibC();
        exe.linkLibCpp();
    } else {`;

code = code.replace(search, replace);

fs.writeFileSync('build.zig', code);
