const fs = require('fs');
let code = fs.readFileSync('build.zig', 'utf8');

const search = `    } else if (t.os.tag == .windows) {
        exe.linkSystemLibrary("stdc++");
        exe.linkSystemLibrary("unwind");
    }`;

const replace = `    } else if (t.os.tag == .windows) {
        // use libc++ on Windows via Zig
        exe.linkLibCpp();
    }`;

code = code.replace(search, replace);
fs.writeFileSync('build.zig', code);
