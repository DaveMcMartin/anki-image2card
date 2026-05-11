const fs = require('fs');
let code = fs.readFileSync('build.zig', 'utf8');

const search = `    } else if (t.os.tag == .windows) {
        // When targeting msvc, zig needs linkLibCpp to properly pull in MSVC C++ runtime dependencies if building C++ source.
        exe.linkLibCpp();
    } else {
        exe.linkLibCpp();
    }`;

const replace = `    } else if (t.os.tag == .windows) {
        // When targeting MSVC with zig build-exe, linkLibCpp causes problems because it tries to link libc++ which clashes with MSVC's STL.
        // MSVC compiler automatically links its C++ standard library.
    } else {
        exe.linkLibCpp();
    }`;

code = code.replace(search, replace);

fs.writeFileSync('build.zig', code);
