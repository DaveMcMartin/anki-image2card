const fs = require('fs');
let code = fs.readFileSync('build.zig', 'utf8');
code = code.replace(`        exe.linkSystemLibrary("windowsapp");
        exe.linkSystemLibrary("windowsapp");`, `        exe.linkSystemLibrary("windowsapp");`);
fs.writeFileSync('build.zig', code);
