const fs = require('fs');
let code = fs.readFileSync('build.zig', 'utf8');

const search = `    exe.addIncludePath(b.path("third_party/sqlite"));
    if (t.os.tag == .windows) {
        exe.addIncludePath(.{ .cwd_relative = "C:/vcpkg/installed/x64-windows/include" });
        exe.addIncludePath(.{ .cwd_relative = "C:/vcpkg/installed/x64-windows/include/leptonica" });
        exe.addIncludePath(.{ .cwd_relative = "C:/vcpkg/installed/x64-windows/include/webp" });
        exe.addLibraryPath(.{ .cwd_relative = "C:/vcpkg/installed/x64-windows/lib" });
        exe.linkSystemLibrary("ole32");
        exe.linkSystemLibrary("oleaut32");
    }`;

const replace = `    exe.addIncludePath(b.path("third_party/sqlite"));
    if (t.os.tag == .windows) {
        exe.addIncludePath(.{ .cwd_relative = "C:/vcpkg/installed/x64-windows/include" });
        exe.addIncludePath(.{ .cwd_relative = "C:/vcpkg/installed/x64-windows/include/leptonica" });
        exe.addIncludePath(.{ .cwd_relative = "C:/vcpkg/installed/x64-windows/include/webp" });
        // Manually link the SDK include path for windows build so it finds winrt and atl headers
        exe.addIncludePath(.{ .cwd_relative = "C:/Program Files (x86)/Windows Kits/10/Include/10.0.22621.0/cppwinrt" });
        exe.addIncludePath(.{ .cwd_relative = "C:/Program Files (x86)/Windows Kits/10/Include/10.0.22621.0/um" });
        exe.addIncludePath(.{ .cwd_relative = "C:/Program Files (x86)/Windows Kits/10/Include/10.0.22621.0/shared" });
        exe.addIncludePath(.{ .cwd_relative = "C:/Program Files (x86)/Windows Kits/10/Include/10.0.22621.0/winrt" });
        exe.addIncludePath(.{ .cwd_relative = "C:/Program Files/Microsoft Visual Studio/2022/Enterprise/VC/Tools/MSVC/14.41.34120/atlmfc/include" });
        exe.addLibraryPath(.{ .cwd_relative = "C:/vcpkg/installed/x64-windows/lib" });
        exe.linkSystemLibrary("ole32");
        exe.linkSystemLibrary("oleaut32");
    }`;

code = code.replace(search, replace);

const search2 = `        exe.linkSystemLibrary("mfuuid");
        exe.linkSystemLibrary("mfplat");
        exe.linkSystemLibrary("mfreadwrite");
        exe.linkSystemLibrary("propsys");`;

const replace2 = `        exe.linkSystemLibrary("windowsapp");
        exe.linkSystemLibrary("mfuuid");
        exe.linkSystemLibrary("mfplat");
        exe.linkSystemLibrary("mfreadwrite");
        exe.linkSystemLibrary("propsys");`;

code = code.replace(search2, replace2);

fs.writeFileSync('build.zig', code);
