const fs = require('fs');
let code = fs.readFileSync('build.zig', 'utf8');

const search = `        exe.addIncludePath(.{ .cwd_relative = "C:/vcpkg/installed/x64-windows/include" });
        exe.addLibraryPath(.{ .cwd_relative = "C:/vcpkg/installed/x64-windows/lib" });`;
const replace = `        exe.addIncludePath(.{ .cwd_relative = "C:/vcpkg/installed/x64-windows/include" });
        exe.addIncludePath(.{ .cwd_relative = "C:/vcpkg/installed/x64-windows/include/leptonica" });
        exe.addIncludePath(.{ .cwd_relative = "C:/vcpkg/installed/x64-windows/include/webp" });
        exe.addLibraryPath(.{ .cwd_relative = "C:/vcpkg/installed/x64-windows/lib" });`;
code = code.replace(search, replace);
fs.writeFileSync('build.zig', code);
