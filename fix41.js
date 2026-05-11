const fs = require('fs');
let code = fs.readFileSync('build.zig', 'utf8');

const search = `        exe.linkLibC();
    } else {
        exe.linkLibC();
        exe.linkLibCpp();
    }`;

const replace = `        exe.linkLibC();
    } else {
        exe.linkLibC();
        exe.linkLibCpp();
    }`;

code = code.replace(search, replace);

fs.writeFileSync('build.zig', code);
