const fs = require('fs');
let code = fs.readFileSync('build.zig', 'utf8');

const search = `    } else if (t.os.tag == .windows) {
        exe.linkSystemLibrary("ws2_32");
        exe.linkSystemLibrary("crypt32");
        exe.linkSystemLibrary("bcrypt");
        exe.linkSystemLibrary("secur32");
        exe.linkSystemLibrary("shlwapi");
        exe.linkSystemLibrary("windowsapp");
    }`;

const replace = `    } else if (t.os.tag == .windows) {
        exe.linkSystemLibrary("ws2_32");
        exe.linkSystemLibrary("crypt32");
        exe.linkSystemLibrary("bcrypt");
        exe.linkSystemLibrary("secur32");
        exe.linkSystemLibrary("shlwapi");
        exe.linkSystemLibrary("windowsapp");
    }`; // Note: it failed looking for "windowsapp". But `windowsapp.lib` should be in the windows SDK.
        // Actually wait, zig links windows SDK by default. If we say `exe.linkSystemLibrary("windowsapp");`, it looks for `windowsapp.lib`.

code = code.replace(search, replace);
fs.writeFileSync('build.zig', code);
