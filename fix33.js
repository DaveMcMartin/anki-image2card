const fs = require('fs');
let code = fs.readFileSync('build.zig', 'utf8');

const search = `    } else {
        exe.linkLibC();
        exe.linkLibCpp();
    }`;

const replace = `    } else {
        exe.linkLibC();
        exe.linkLibCpp();
    }`;

code = code.replace(search, replace);

fs.writeFileSync('build.zig', code);
