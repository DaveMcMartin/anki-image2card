const fs = require('fs');
let code = fs.readFileSync('build.zig', 'utf8');

const search = `    } else if (t.os.tag == .windows) {
        src_files.append("src/ai/native/NativeAudioProvider_Windows.cpp") catch @panic("OOM");
        src_files.append("src/ocr/native/NativeOCRProvider_Windows.cpp") catch @panic("OOM");
    }`;

const replace = `    } else if (t.os.tag == .windows) {
        src_files.append("src/ai/native/NativeAudioProvider_Windows.cpp") catch @panic("OOM");
        src_files.append("src/ocr/native/NativeOCRProvider_Windows.cpp") catch @panic("OOM");
    }`;

code = code.replace(search, replace);
fs.writeFileSync('build.zig', code);
