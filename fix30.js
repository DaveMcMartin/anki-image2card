const fs = require('fs');
let code = fs.readFileSync('build.zig', 'utf8');

const search = `    } else if (t.os.tag == .windows) {
        // When targeting MSVC with zig build-exe, linkLibCpp causes problems because it tries to link libc++ which clashes with MSVC's STL.
        // MSVC compiler automatically links its C++ standard library if we don't supply -nostdinc++ via linkLibCpp().
        // actually zig explicitly handles msvc c++ via simple C linkage plus MSVC args internally so we don't need linkLibCpp.
    } else {`;

const replace = `    } else if (t.os.tag == .windows) {
        // When targeting MSVC with zig build-exe, linkLibC() pulls in msvcrt automatically.
        exe.linkLibC();
    } else {`;

code = code.replace(search, replace);

fs.writeFileSync('build.zig', code);
