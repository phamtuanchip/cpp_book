// Bien dich + chay moi code/chapter-*/*.cpp (g++ -std=c++20 -Wall -Wextra), ghi output vao <file>.runout.txt.
// Mac dinh chi BIEN DICH (-fsyntax-only: bat loi cu phap, canh bao, static_assert). Them --run de chay va ghi output.
// Loi bien dich hoac canh bao => that bai. Dung:  node tools/verify.js [--run] [chapter-01 ...]
'use strict';
const fs = require('fs');
const os = require('os');
const path = require('path');
const { spawnSync } = require('child_process');

const CODE = path.join(__dirname, '..', 'code');
const CXX = process.env.CXX || 'g++';
const args = process.argv.slice(2);
const RUN = args.includes('--run');
const only = args.filter((a) => !a.startsWith('--'));
const tmp = fs.mkdtempSync(path.join(os.tmpdir(), 'cppbook-'));
const ext = process.platform === 'win32' ? '.exe' : '';
const sleep = (ms) => Atomics.wait(new Int32Array(new SharedArrayBuffer(4)), 0, 0, ms);

let failed = 0, n = 0;
for (const dir of fs.readdirSync(CODE).filter((d) => /^chapter-\d+$/.test(d)).sort()) {
  if (only.length && !only.includes(dir)) continue;
  for (const f of fs.readdirSync(path.join(CODE, dir)).filter((x) => x.endsWith('.cpp')).sort()) {
    const src = path.join(CODE, dir, f);
    n++;
    const exe = path.join(tmp, `${dir}-${f.replace(/.cpp$/, '')}${ext}`);
    const cc = spawnSync(CXX, RUN ? ['-std=c++20', '-Wall', '-Wextra', '-o', exe, src] : ['-std=c++20', '-Wall', '-Wextra', '-fsyntax-only', src], { encoding: 'utf8' });
    if (cc.error) { console.error(`Khong chay duoc ${CXX}: ${cc.error.message}`); process.exit(2); }
    if (cc.status !== 0 || cc.stderr.trim()) {
      failed++;
      console.error(`FAIL ${dir}/${f}\n${cc.stderr}`);
      continue;
    }
    if (!RUN) { console.log(`ok   ${dir}/${f} (bien dich)`); continue; }
    // Tren Windows, exe moi link co the bi khoa tam thoi (antivirus quet) -> thu lai.
    let run;
    for (let i = 0; i < 6; i++) {
      run = spawnSync(exe, [], { encoding: 'utf8' });
      if (run.status !== null) break;
      sleep(300);
    }
    if (run.status !== 0) { failed++; console.error(`FAIL (exit ${run.status}, signal ${run.signal}, error ${run.error && run.error.code}) ${dir}/${f}`); continue; }
    fs.writeFileSync(src.replace(/\.cpp$/, '.runout.txt'), run.stdout);
    console.log(`ok   ${dir}/${f}`);
  }
}
fs.rmSync(tmp, { recursive: true, force: true });
console.log(`${n - failed}/${n} vi du dat.`);
process.exit(failed ? 1 : 0);
