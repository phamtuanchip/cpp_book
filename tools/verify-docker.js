// Bien dich + chay moi vi du trong container gcc, thay cho g++ tren may that.
// Dung khi may khong chay duoc g++ truc tiep (vd Windows bat Smart App Control chan file khong ky).
// Dung:  node tools/verify-docker.js [--no-run] [gcc:14]
'use strict';
const path = require('path');
const { spawnSync } = require('child_process');

const ROOT = path.join(__dirname, '..');
const args = process.argv.slice(2);
const RUN = !args.includes('--no-run');
const IMAGE = args.find((a) => !a.startsWith('--')) || 'gcc:14';

// Script chay BEN TRONG container: chi can bash + g++, khong can node.
const script = `
set -u
fail=0; n=0
for f in code/chapter-*/*.cpp; do
  n=$((n+1))
  if ! out=$(g++ -std=c++20 -Wall -Wextra -o /tmp/a.out "$f" 2>&1); then
    echo "FAIL bien dich: $f"; echo "$out" | head -20; fail=$((fail+1)); continue
  fi
  if [ -n "$out" ]; then
    echo "FAIL canh bao:  $f"; echo "$out" | head -20; fail=$((fail+1)); continue
  fi
  ${RUN ? `
  if ! /tmp/a.out > "\${f%.cpp}.runout.txt" 2>/tmp/err; then
    echo "FAIL chay:      $f"; head -5 /tmp/err; fail=$((fail+1)); continue
  fi
  echo "ok   $f"
  ` : 'echo "ok   $f (bien dich)"'}
done
echo "$((n-fail))/$n vi du dat."
exit $((fail > 0))
`;

// Duong dan tuyet doi cho -v: tren Windows can dang //c/Users/... de Docker hieu dung.
const mount = process.platform === 'win32'
  ? '//' + ROOT.replace(/\\/g, '/').replace(/^([A-Za-z]):/, (_, d) => d.toLowerCase())
  : ROOT;

const r = spawnSync('docker', ['run', '--rm', '-v', `${mount}://work`, '-w', '//work', IMAGE, 'bash', '-c', script],
  { stdio: 'inherit' });

if (r.error) {
  console.error(`Khong chay duoc docker: ${r.error.message}`);
  console.error('Kiem tra Docker Desktop da khoi dong chua (docker info).');
  process.exit(2);
}
process.exit(r.status === null ? 1 : r.status);
