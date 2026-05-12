const fs = require('fs');

function updateFile(file) {
  let content = fs.readFileSync(file, 'utf8');

  // We are currently doing zig build -Dtarget=x86_64-windows-gnu
  // wait we reverted back to windows-gnu. Let's make sure it's correct.

}
