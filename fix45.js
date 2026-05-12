const fs = require('fs');
let code = fs.readFileSync('build.zig', 'utf8');

const search = `        exe.addIncludePath(.{ .cwd_relative = "C:/vcpkg/installed/x64-windows/include" });
        exe.addIncludePath(.{ .cwd_relative = "C:/vcpkg/installed/x64-windows/include/leptonica" });
        exe.addIncludePath(.{ .cwd_relative = "C:/vcpkg/installed/x64-windows/include/webp" });
        exe.addIncludePath(.{ .cwd_relative = "C:/vcpkg/installed/x64-windows/include/mecab" });`;

const replace = `        exe.addIncludePath(.{ .cwd_relative = "C:/vcpkg/installed/x64-windows/include" });
        exe.addIncludePath(.{ .cwd_relative = "C:/vcpkg/installed/x64-windows/include/leptonica" });
        exe.addIncludePath(.{ .cwd_relative = "C:/vcpkg/installed/x64-windows/include/webp" });
        exe.addIncludePath(.{ .cwd_relative = "C:/vcpkg/installed/x64-windows/include/mecab" });

        exe.addLibraryPath(.{ .cwd_relative = "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.22621.0/um/x64" });
        exe.addLibraryPath(.{ .cwd_relative = "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/um/x64" });
        exe.addLibraryPath(.{ .cwd_relative = "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.22621.0/ucrt/x64" });
        exe.addLibraryPath(.{ .cwd_relative = "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/ucrt/x64" });`;

code = code.replace(search, replace);
fs.writeFileSync('build.zig', code);
