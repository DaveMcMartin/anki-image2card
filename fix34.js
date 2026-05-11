const fs = require('fs');
let code = fs.readFileSync('build.zig', 'utf8');

const search = `        exe.linkSystemLibrary("shlwapi");
        exe.linkSystemLibrary("ole32");
        exe.linkSystemLibrary("oleaut32");
        exe.linkSystemLibrary("windowsapp");
        exe.linkSystemLibrary("mfuuid");
        exe.linkSystemLibrary("mfplat");
        exe.linkSystemLibrary("mfreadwrite");
        exe.linkSystemLibrary("propsys");`;

const replace = `        exe.linkSystemLibrary("shlwapi");
        exe.linkSystemLibrary("ole32");
        exe.linkSystemLibrary("oleaut32");
        exe.linkSystemLibrary("mfuuid");
        exe.linkSystemLibrary("mfplat");
        exe.linkSystemLibrary("mfreadwrite");
        exe.linkSystemLibrary("propsys");`;

code = code.replace(search, replace);

fs.writeFileSync('build.zig', code);
