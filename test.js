const fs = require('fs');
let code = fs.readFileSync('build.zig', 'utf8');
const search = `    exe.addIncludePath(b.path("third_party/sqlite"));`;
const replace = `    exe.addIncludePath(b.path("third_party/sqlite"));
    if (t.os.tag == .windows) {
        exe.addIncludePath(.{ .cwd_relative = "C:/vcpkg/installed/x64-windows/include" });
        exe.addLibraryPath(.{ .cwd_relative = "C:/vcpkg/installed/x64-windows/lib" });
    }`;

code = code.replace(search, replace);
fs.writeFileSync('build.zig', code);
