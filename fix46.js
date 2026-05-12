const fs = require('fs');
let code = fs.readFileSync('build.zig', 'utf8');

const search = `        exe.linkSystemLibrary("ole32");
        exe.linkSystemLibrary("oleaut32");
        exe.linkSystemLibrary("mfuuid");`;

const replace = `        exe.linkSystemLibrary("ole32");
        exe.linkSystemLibrary("oleaut32");
        exe.linkSystemLibrary("windowsapp");
        exe.linkSystemLibrary("mfuuid");`;

code = code.replace(search, replace);
fs.writeFileSync('build.zig', code);
