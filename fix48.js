const fs = require('fs');
let code = fs.readFileSync('build.zig', 'utf8');

const search = `    const cpp_flags = &[_][]const u8{
        "-std=c++23",
        "-fexceptions",
        "-DCPPHTTPLIB_OPENSSL_SUPPORT",
        "-O2"
    };`;

const replace = `    const cpp_flags = &[_][]const u8{
        "-std=c++23",
        "-fexceptions",
        "-DCPPHTTPLIB_OPENSSL_SUPPORT",
        "-DNOMINMAX",
        "-DWIN32_LEAN_AND_MEAN",
        "-O2"
    };`;

code = code.replace(search, replace);

fs.writeFileSync('build.zig', code);
