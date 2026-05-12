const fs = require('fs');

function updateFile(file) {
  let content = fs.readFileSync(file, 'utf8');

  const oldSDL = `          export CFLAGS=-IC:/SDL3/SDL3-3.1.3/include
          export LDFLAGS=-LC:/SDL3/SDL3-3.1.3/lib
          export PKG_CONFIG_PATH=C:/vcpkg/installed/x64-windows/lib/pkgconfig
          rm -rf ~/.cache/zig .zig-cache && zig build -Dtarget=x86_64-windows-gnu -Doptimize=ReleaseFast -fno-reference-trace --search-prefix "C:/SDL3/SDL3-3.1.3" --search-prefix "C:/vcpkg/installed/x64-windows"`;

  const newSDL = `          export CFLAGS=-IC:/SDL3/SDL3-3.1.3/include
          export LDFLAGS=-LC:/SDL3/SDL3-3.1.3/lib
          export PKG_CONFIG_PATH=C:/vcpkg/installed/x64-windows/lib/pkgconfig
          rm -rf ~/.cache/zig .zig-cache && zig build -Dtarget=x86_64-windows-msvc -Doptimize=ReleaseFast -fno-reference-trace --search-prefix "C:/SDL3/SDL3-3.1.3" --search-prefix "C:/vcpkg/installed/x64-windows"`;

  content = content.replace(oldSDL, newSDL);
  fs.writeFileSync(file, content);
}

updateFile('.github/workflows/pr.yml');
updateFile('.github/workflows/release.yml');
