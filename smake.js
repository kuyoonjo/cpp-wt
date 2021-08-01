const { LLVM_Darwin, LLVM_Linux, LLVM_Win32 } = require('smake');
const { readdirSync } = require('fs');
const { join } = require('path');

const items = readdirSync(join(__dirname, 'test'))
  .filter(x => x.endsWith('.cc'))
  .map(x => x.replace(/\.cc$/, ''));

const targets = items.map(item => {
  const darwin_x86_64 = class extends LLVM_Darwin {
    includedirs = [
      ...super.includedirs,
      './include',
      './MPMCQueue/include',
    ];
    cxflags = [
      ...super.cxflags,
      '-std=c++2a'
    ];
    files = ['test/' + item + '.cc'];
  };
  Object.defineProperty(darwin_x86_64, 'name', { value: item + '_darwin_x86_64' });
  
  const linux_arm64 = class extends LLVM_Linux {
    target = 'aarch64-linux-gnu';
    includedirs = [
      ...super.includedirs,
      './include',
      './MPMCQueue/include',
    ];
    ldflags = [
      ...super.ldflags,
      '-pthread',
      '-static-libstdc++',
    ];
    files = ['test/' + item + '.cc'];
  };
  Object.defineProperty(linux_arm64, 'name', { value: item + '_linux_arm64' });
  
  const win_x86_64 = class extends LLVM_Win32 {
    ARCH = 'x86_64';
    useClangHeaders = true;
    includedirs = [
      ...super.includedirs,
      './include',
      './MPMCQueue/include',
    ];
    cxflags = [
      ...super.cxflags,
      '-D_WIN32_WINNT=0x0A00',
      '-D_CRT_SECURE_NO_WARNINGS',
    ];
    ldflags = [
      ...super.ldflags,
    ];
    files = ['test/' + item + '.cc'];
  };
  Object.defineProperty(win_x86_64, 'name', { value: item + '_win32_x86_64' });
  return [darwin_x86_64, linux_arm64, win_x86_64];
}).reduce((a, b) => a.concat(b), []);

module.exports = {
  targets,
};
