const fs = require('fs');

function updateFile(file) {
  let content = fs.readFileSync(file, 'utf8');

  const oldSDL = `          $SDL_URL = "https://github.com/libsdl-org/SDL/releases/download/preview-3.1.3/SDL3-devel-3.1.3-mingw.zip"
          Invoke-WebRequest -Uri $SDL_URL -OutFile SDL3.zip
          Expand-Archive SDL3.zip -DestinationPath C:\\SDL3
          $env:PATH += ";C:\\SDL3\\SDL3-3.1.3\\x86_64-w64-mingw32\\bin"
          echo "PATH=$env:PATH" >> $env:GITHUB_ENV

      - name: Build (Windows)
        if: runner.os == 'Windows'
        run: |
          export CFLAGS=-IC:/SDL3/SDL3-3.1.3/x86_64-w64-mingw32/include
          export LDFLAGS=-LC:/SDL3/SDL3-3.1.3/x86_64-w64-mingw32/lib
          export PKG_CONFIG_PATH=C:/vcpkg/installed/x64-windows/lib/pkgconfig
          rm -rf ~/.cache/zig .zig-cache && zig build -Doptimize=ReleaseFast -fno-reference-trace --search-prefix "C:/SDL3/SDL3-3.1.3/x86_64-w64-mingw32" --search-prefix "C:/vcpkg/installed/x64-windows"`;

  const newSDL = `          $SDL_URL = "https://github.com/libsdl-org/SDL/releases/download/preview-3.1.3/SDL3-devel-3.1.3-VC.zip"
          Invoke-WebRequest -Uri $SDL_URL -OutFile SDL3.zip
          Expand-Archive SDL3.zip -DestinationPath C:\\SDL3

          # HACK: Move the x64 libs to the root /lib directory so Zig's --search-prefix finds them automatically
          Copy-Item -Path "C:\\SDL3\\SDL3-3.1.3\\lib\\x64\\*" -Destination "C:\\SDL3\\SDL3-3.1.3\\lib\\" -Recurse

          $env:PATH += ";C:\\SDL3\\SDL3-3.1.3\\lib\\x64"
          echo "PATH=$env:PATH" >> $env:GITHUB_ENV

      - name: Build (Windows)
        if: runner.os == 'Windows'
        run: |
          export CFLAGS=-IC:/SDL3/SDL3-3.1.3/include
          export LDFLAGS=-LC:/SDL3/SDL3-3.1.3/lib
          export PKG_CONFIG_PATH=C:/vcpkg/installed/x64-windows/lib/pkgconfig

          # CHANGED: Target MSVC (-Dtarget=x86_64-windows-msvc) and update search prefixes
          rm -rf ~/.cache/zig .zig-cache && zig build -Dtarget=x86_64-windows-msvc -Doptimize=ReleaseFast -fno-reference-trace --search-prefix "C:/SDL3/SDL3-3.1.3" --search-prefix "C:/vcpkg/installed/x64-windows"`;

  content = content.replace(oldSDL, newSDL);
  fs.writeFileSync(file, content);
}

updateFile('.github/workflows/pr.yml');
updateFile('.github/workflows/release.yml');
