const fs = require('fs');
let code = fs.readFileSync('build.zig', 'utf8');

const search = `    } else if (t.os.tag == .windows) {
        exe.linkLibC();
        exe.linkSystemLibrary("stdc++");
    } else {`;

const replace = `    } else if (t.os.tag == .windows) {
        // use MSVC libc++ instead of standard zig one since we target MSVC.
        // Actually, linkSystemLibrary("stdc++") isn't right for msvc.
        // MSVC links libcmt implicitly, and libc++ implicitly via its setup.
        // Zig MSVC target will automatically link correct C++ library if we just call linkLibC() / compile C++.
        // We do NOT want stdc++ for windows-msvc target. It fails with: "unable to find library stdc++" or links GNU stuff causing ABI issues.
        exe.linkLibC();
        exe.linkLibCpp();
    } else {`;

code = code.replace(search, replace);

fs.writeFileSync('build.zig', code);
