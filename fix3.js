const fs = require('fs');
let code = fs.readFileSync('build.zig', 'utf8');

const search = `    if (t.os.tag == .windows) {
        exe.linkSystemLibrary("ws2_32");
        exe.linkSystemLibrary("crypt32");
        exe.linkSystemLibrary("bcrypt");
        exe.linkSystemLibrary("secur32");
        exe.linkSystemLibrary("shlwapi");`;

const replace = `    if (t.os.tag == .windows) {
        exe.linkSystemLibrary("ws2_32");
        exe.linkSystemLibrary("crypt32");
        exe.linkSystemLibrary("bcrypt");
        exe.linkSystemLibrary("secur32");
        exe.linkSystemLibrary("shlwapi");
        exe.linkSystemLibrary("windowsapp");`;

code = code.replace(search, replace);
fs.writeFileSync('build.zig', code);
