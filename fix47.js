const fs = require('fs');

function updateFile(file) {
  let content = fs.readFileSync(file, 'utf8');

  // We are currently doing zig build -Dtarget=x86_64-windows-msvc but it seems to fail with a lot of missing MSVC ABI components when we are linking stdc++
  // Or rather the error log told us we had compilation errors in Windows.Globalization.h "min" because of macro clashes in windows headers.
  // We need to fix the macro clash.

  // Let's not modify the cpp files, let's just make sure -DNOMINMAX is passed to the compiler.
  // Actually wait, let me check build.zig to add NOMINMAX flag.
}
