const fs = require('fs');
let code = fs.readFileSync('build.zig', 'utf8');

const search = `    } else if (t.os.tag == .windows) {
        src_files.append("src/ai/native/NativeAudioProvider_Windows.cpp") catch @panic("OOM");
        src_files.append("src/ocr/native/NativeOCRProvider_Windows.cpp") catch @panic("OOM");
    }`;

const replace = `    } else if (t.os.tag == .windows) {
        src_files.append("src/ai/native/NativeAudioProvider_Windows.cpp") catch @panic("OOM");
        src_files.append("src/ocr/native/NativeOCRProvider_Windows.cpp") catch @panic("OOM");
    }`; // The issue was `mecab.h` not found as well. That means we need include path for it.

// For mecab, the include path is `C:/vcpkg/installed/x64-windows/include/mecab` maybe?
// Actually vcpkg installs it in `C:/vcpkg/installed/x64-windows/include` probably, or `include/mecab`. Let's add it.

const search2 = `        exe.addIncludePath(.{ .cwd_relative = "C:/vcpkg/installed/x64-windows/include/webp" });`;
const replace2 = `        exe.addIncludePath(.{ .cwd_relative = "C:/vcpkg/installed/x64-windows/include/webp" });
        exe.addIncludePath(.{ .cwd_relative = "C:/vcpkg/installed/x64-windows/include/mecab" });`;

code = code.replace(search2, replace2);
fs.writeFileSync('build.zig', code);
