const fs = require('fs');
let code = fs.readFileSync('build.zig', 'utf8');

const search = `        exe.linkSystemLibrary("libwebp");
        exe.linkSystemLibrary("mecab");`;

const replace = `        exe.linkSystemLibrary("webp");
        exe.linkSystemLibrary("mecab");`;

code = code.replace(search, replace);

fs.writeFileSync('build.zig', code);
