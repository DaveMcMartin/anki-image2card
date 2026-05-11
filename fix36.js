const fs = require('fs');
let yml = fs.readFileSync('.github/workflows/pr.yml', 'utf8');

const search = `          rm -rf ~/.cache/zig .zig-cache && zig build -Dtarget=x86_64-windows-msvc -Doptimize=ReleaseFast -fno-reference-trace --search-prefix "C:/SDL3/SDL3-3.1.3" --search-prefix "C:/vcpkg/installed/x64-windows"`;

const replace = `          rm -rf ~/.cache/zig .zig-cache && zig build -Dtarget=x86_64-windows-gnu -Doptimize=ReleaseFast -fno-reference-trace --search-prefix "C:/SDL3/SDL3-3.1.3" --search-prefix "C:/vcpkg/installed/x64-windows"`;

yml = yml.replace(search, replace);
fs.writeFileSync('.github/workflows/pr.yml', yml);

let yml2 = fs.readFileSync('.github/workflows/release.yml', 'utf8');
yml2 = yml2.replace(search, replace);
fs.writeFileSync('.github/workflows/release.yml', yml2);
