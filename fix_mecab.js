const fs = require('fs');
let code = fs.readFileSync('build.zig', 'utf8');

const search = `        exe.linkSystemLibrary("windowsapp");
        exe.linkSystemLibrary("mecab");
    }`;

const replace = `        exe.linkSystemLibrary("windowsapp");
    }`;

code = code.replace(search, replace);
fs.writeFileSync('build.zig', code);
