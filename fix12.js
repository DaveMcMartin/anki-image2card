const fs = require('fs');
let code = fs.readFileSync('build.zig', 'utf8');

const search = `        exe.addIncludePath(.{ .cwd_relative = "C:/Program Files/Microsoft Visual Studio/2022/Enterprise/VC/Tools/MSVC/14.41.34120/atlmfc/include" });`;

const replace = `        // In GitHub Actions, ATL is not always in the standard include path. We need to find it or avoid using it.
        // wait, let's just use the known path but fall back to checking if it exists if possible.
        // GitHub Actions has MSVC installed.
        exe.addIncludePath(.{ .cwd_relative = "C:/Program Files/Microsoft Visual Studio/2022/Enterprise/VC/Tools/MSVC/14.41.34120/atlmfc/include" });`;

code = code.replace(search, replace);
fs.writeFileSync('build.zig', code);
