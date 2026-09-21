// Bien dich + chay moi code/chapter-*/*.cpp (g++ -std=c++20 -Wall -Wextra), ghi output vao <file>.runout.txt.
// Loi bien dich hoac canh bao => that bai. Dung:  node tools/verify.js [chapter-01 ...]
'use strict';
const fs = require('fs');
const os = require('os');
const path = require('path');
const { spawnSync } = require('child_process');

const CODE = path.join(__dirname, '..', 'code');
const CXX = process.env.CXX || 'g++';
const only = process.argv.slice(2);
const tmp = fs.mkdtempSync(path.join(os.tmpdir(), 'cppbook-'));
const exe = path.join(tmp, process.platform === 'win32' ? 'a.exe' : 'a.out');

let failed = 0, n = 0;
for (const dir of fs.readdirSync(CODE).filter((d) => /^chapter-\d+$/.test(d)).sort()) {
  if (only.length && !only.includes(dir)) continue;
  for (const f of fs.readdirSync(path.join(CODE, dir)).filter((x) => x.endsWith('.cpp')).sort()) {
    const src = path.join(CODE, dir, f);
    n++;
    const cc = spawnSync(CXX, ['-std=c++20', '-Wall', '-Wextra', '-o', exe, src], { encoding: 'utf8' });
    if (cc.error) { console.error(`Khong chay duoc ${CXX}: ${cc.error.message}`); process.exit(2); }
    if (cc.status !== 0 || cc.stderr.trim()) {
      failed++;
      console.error(`FAIL ${dir}/${f}\n${cc.stderr}`);
      continue;
    }
    const run = spawnSync(exe, [], { encoding: 'utf8' });
    if (run.status !== 0) { failed++; console.error(`FAIL (exit ${run.status}) ${dir}/${f}`); continue; }
    fs.writeFileSync(src.replace(/\.cpp$/, '.runout.txt'), run.stdout);
    console.log(`ok   ${dir}/${f}`);
  }
}
fs.rmSync(tmp, { recursive: true, force: true });
console.log(`${n - failed}/${n} vi du dat.`);
process.exit(failed ? 1 : 0);
