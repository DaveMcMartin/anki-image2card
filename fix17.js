const fs = require('fs');
let code = fs.readFileSync('build.zig', 'utf8');

const search = `        exe.addIncludePath(.{ .cwd_relative = "C:/Program Files/Microsoft Visual Studio/2022/Enterprise/VC/Tools/MSVC/14.41.34120/atlmfc/include" });
        exe.addLibraryPath(.{ .cwd_relative = "C:/vcpkg/installed/x64-windows/lib" });`;

const replace = `        exe.addIncludePath(.{ .cwd_relative = "C:/Program Files/Microsoft Visual Studio/2022/Enterprise/VC/Tools/MSVC/14.41.34120/atlmfc/include" });
        exe.addLibraryPath(.{ .cwd_relative = "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.22621.0/um/x64" });
        exe.addLibraryPath(.{ .cwd_relative = "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.22621.0/ucrt/x64" });
        exe.addLibraryPath(.{ .cwd_relative = "C:/vcpkg/installed/x64-windows/lib" });`;

code = code.replace(search, replace);

const search2 = `        exe.linkSystemLibrary("mfuuid");`;
const replace2 = `        exe.linkSystemLibrary("windowsapp");
        exe.linkSystemLibrary("mfuuid");`;

code = code.replace(search2, replace2);

fs.writeFileSync('build.zig', code);

let yml = fs.readFileSync('.github/workflows/pr.yml', 'utf8');

const ymlSearch = `          $SDL_URL = "https://github.com/libsdl-org/SDL/releases/download/preview-3.1.3/SDL3-devel-3.1.3-mingw.zip"
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

const ymlReplace = `          $SDL_URL = "https://github.com/libsdl-org/SDL/releases/download/preview-3.1.3/SDL3-devel-3.1.3-VC.zip"
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
          rm -rf ~/.cache/zig .zig-cache && zig build -Dtarget=x86_64-windows-msvc -Doptimize=ReleaseFast -fno-reference-trace --search-prefix "C:/SDL3/SDL3-3.1.3" --search-prefix "C:/vcpkg/installed/x64-windows"`;

yml = yml.replace(ymlSearch, ymlReplace);
fs.writeFileSync('.github/workflows/pr.yml', yml);

let yml2 = fs.readFileSync('.github/workflows/release.yml', 'utf8');
yml2 = yml2.replace(ymlSearch, ymlReplace);
fs.writeFileSync('.github/workflows/release.yml', yml2);
