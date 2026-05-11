const fs = require('fs');
let code = fs.readFileSync('build.zig', 'utf8');
code = code.replace(`        exe.linkSystemLibrary("SDL3");
        exe.addLibraryPath(.{ .cwd_relative = "/usr/local/lib" });`, `        exe.linkSystemLibrary("SDL3");
        exe.addLibraryPath(.{ .cwd_relative = "/usr/local/lib" });`);
// wait we need to make sure SDL3 linker in windows uses the correct msbuild name instead of standard SDL3?
// No, vcpkg/SDL3-VC provides SDL3.lib
// Is there anything else? The user says the main issue was ABI clash.
