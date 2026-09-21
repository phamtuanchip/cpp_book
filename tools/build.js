// Build sach tu manuscript/**.md -> dist/ (HTML, GitHub Pages) va formats/book.pdf (Chrome headless).
// Dung:  node tools/build.js html | pdf | check | all
'use strict';

const fs = require('fs');
const path = require('path');
const { execFileSync } = require('child_process');
const MarkdownIt = require('markdown-it');
const hljs = require('highlight.js');

const ROOT = path.join(__dirname, '..');
const SRC = path.join(ROOT, 'manuscript');
const DIST = path.join(ROOT, 'dist');
const FORMATS = path.join(ROOT, 'formats');
const REPO = 'https://github.com/phamtuanchip/cpp_book';

const meta = readMeta(path.join(ROOT, 'metadata.yaml'));
const BOOK_TITLE = `${meta.title} — ${meta.subtitle}`;

const md = new MarkdownIt({
  html: false,
  linkify: true,
  highlight(str, lang) {
    const l = lang === 'cpp' || lang === 'c++' ? 'cpp' : lang;
    const body = l && hljs.getLanguage(l) ? hljs.highlight(str, { language: l }).value : md.utils.escapeHtml(str);
    return `<pre class="hljs"><code>${body}</code></pre>`;
  },
});

// ---------- doc nguon ----------
function readMeta(file) {
  const out = {};
  for (const line of fs.readFileSync(file, 'utf8').split(/\r?\n/)) {
    const m = line.match(/^(\w+):\s*(.+)$/);
    if (m) out[m[1]] = m[2].replace(/^"|"$/g, '');
  }
  return out;
}

function parseFrontMatter(text) {
  text = text.replace(/^﻿/, '');
  const m = text.match(/^---\r?\n([\s\S]*?)\r?\n---\r?\n?/);
  const fm = {};
  if (!m) return { fm, body: text };
  for (const line of m[1].split(/\r?\n/)) {
    const kv = line.match(/^(\w+):\s*(.*)$/);
    if (kv) fm[kv[1]] = kv[2].replace(/^"|"$/g, '');
  }
  return { fm, body: text.slice(m[0].length) };
}

function walk(dir) {
  return fs.readdirSync(dir, { withFileTypes: true }).flatMap((e) =>
    e.isDirectory() ? walk(path.join(dir, e.name)) : [path.join(dir, e.name)]);
}

function loadChapters() {
  return walk(SRC)
    .filter((f) => f.endsWith('.md'))
    .sort((a, b) => path.relative(SRC, a).localeCompare(path.relative(SRC, b)))
    .map((file) => {
      const { fm, body } = parseFrontMatter(fs.readFileSync(file, 'utf8'));
      const rel = path.relative(SRC, file).replace(/\\/g, '/');
      const slug = path.basename(file, '.md');
      const heading = (body.match(/^#\s+(.+)$/m) || [])[1] || slug;
      return {
        file, rel, slug, fm, body,
        part: rel.split('/')[0],
        front: rel.startsWith('00-front-matter/'),
        num: fm.chapter ? Number(fm.chapter) : null,
        title: fm.title || heading,
        href: `${slug}.html`,
      };
    });
}

const PART_NAMES = {
  'part0-nen-tang': 'Phần 0 — Nền tảng và câu chuyện',
  'part1-moi-truong': 'Phần 1 — Môi trường và công cụ',
  'part2-co-ban': 'Phần 2 — C++ cơ bản',
  'part3-oop': 'Phần 3 — OOP cho nhúng',
  'part4-hien-dai': 'Phần 4 — C++ hiện đại',
  'part5-bo-nho': 'Phần 5 — Bộ nhớ và cấp thấp',
  'part6-thuc-chien': 'Phần 6 — Nhúng thực chiến',
  'part7-chat-luong': 'Phần 7 — Chất lượng và hiệu năng',
  'part8-dinh-huong': 'Phần 8 — Định hướng và phụ lục',
  '00-front-matter': 'Mở đầu',
};

// Chen ma nguon that tu code/ vao sach:  {{code:chapter-01/hello.cpp}}  va  {{out:chapter-01/hello}}
function inlineCode(src) {
  return src
    .replace(/\{\{code:([^}]+)\}\}/g, (_, p) => {
      const f = path.join(ROOT, 'code', p);
      if (!fs.existsSync(f)) throw new Error(`Thieu file code: ${p}`);
      const lang = p.endsWith('.h') || p.endsWith('.hpp') || p.endsWith('.cpp') ? 'cpp' : '';
      return '```' + lang + '\n' + fs.readFileSync(f, 'utf8').replace(/\s+$/, '') + '\n```';
    })
    .replace(/\{\{out:([^}]+)\}\}/g, (_, p) => {
      const f = path.join(ROOT, 'code', p + '.runout.txt');
      if (!fs.existsSync(f)) return '> *Chạy ví dụ trên máy bạn để xem kết quả.*';
      return '```text\n' + fs.readFileSync(f, 'utf8').replace(/\s+$/, '') + '\n```';
    });
}

const render = (c) => md.render(inlineCode(c.body));

// ---------- HTML ----------
function sidebar(chapters, active) {
  let h = `<nav class="sidebar"><div class="sidebar-title"><a href="index.html">${meta.title}</a></div><ul class="part-list">`;
  let lastPart = null;
  for (const c of chapters) {
    if (c.part !== lastPart) {
      if (lastPart) h += '</ul></li>';
      h += `<li class="part"><div class="part-title">${PART_NAMES[c.part] || c.part}</div><ul class="chapter-list">`;
      lastPart = c.part;
    }
    const label = c.num ? `${c.num}. ${c.title}` : c.title;
    h += `<li><a class="chapter-link${c.href === active ? ' active' : ''}" href="${c.href}">${label}</a></li>`;
  }
  h += `</ul></li><li class="part"><div class="part-title">Mã nguồn</div><ul class="chapter-list"><li><a class="chapter-link" href="${REPO}/tree/main/code">Code mẫu trên GitHub</a></li></ul></li></ul></nav>`;
  return h;
}

function page({ title, body, chapters, href, prev, next }) {
  return `<!doctype html>
<html lang="vi"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>${title} — ${meta.title}</title><link rel="stylesheet" href="style.css"></head>
<body><div class="layout">${sidebar(chapters, href)}<main class="content"><article>
${body}
</article><div class="chapter-nav">${prev ? `<a class="nav-prev" href="${prev.href}">&larr; ${prev.title}</a>` : '<span></span>'}${next ? `<a class="nav-next" href="${next.href}">${next.title} &rarr;</a>` : '<span></span>'}</div></main></div></body></html>
`;
}

function buildHtml() {
  const chapters = loadChapters();
  fs.rmSync(DIST, { recursive: true, force: true });
  fs.mkdirSync(DIST, { recursive: true });
  fs.copyFileSync(path.join(ROOT, 'templates', 'book.css'), path.join(DIST, 'style.css'));
  if (fs.existsSync(path.join(ROOT, 'assets'))) fs.cpSync(path.join(ROOT, 'assets'), path.join(DIST, 'assets'), { recursive: true });
  chapters.forEach((c, i) => {
    const html = page({ title: c.title, body: render(c), chapters, href: c.href, prev: chapters[i - 1], next: chapters[i + 1] });
    fs.writeFileSync(path.join(DIST, c.href), html, 'utf8');
  });
  const first = chapters[0];
  const indexBody = `<h1>${meta.title}</h1><p class="subtitle">${meta.subtitle}. ${meta.description}</p><p><a class="start-link" href="${first.href}">Bắt đầu đọc &rarr;</a></p><p>Mã nguồn: <a href="${REPO}">${REPO}</a></p>`;
  fs.writeFileSync(path.join(DIST, 'index.html'), `<!doctype html>
<html lang="vi"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1"><title>${meta.title}</title><link rel="stylesheet" href="style.css"></head>
<body><div class="layout">${sidebar(chapters, 'index.html')}<main class="content"><article>${indexBody}</article></main></div></body></html>
`, 'utf8');
  console.log(`HTML: ${chapters.length} trang -> dist/`);
}

// ---------- PDF ----------
function findChrome() {
  const cands = [
    process.env.CHROME_PATH,
    'C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe',
    'C:\\Program Files (x86)\\Google\\Chrome\\Application\\chrome.exe',
    'C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\msedge.exe',
    '/usr/bin/google-chrome', '/usr/bin/chromium', '/usr/bin/chromium-browser',
  ].filter(Boolean);
  const found = cands.find((p) => fs.existsSync(p));
  if (!found) throw new Error('Khong tim thay Chrome/Edge. Dat bien CHROME_PATH.');
  return found;
}

function buildPdf() {
  const chapters = loadChapters();
  const css = fs.readFileSync(path.join(ROOT, 'templates', 'book.css'), 'utf8') +
    '\n' + fs.readFileSync(path.join(ROOT, 'templates', 'print.css'), 'utf8');
  let toc = '<nav class="toc"><h1>Mục lục</h1>';
  let body = '';
  let lastPart = null;
  for (const c of chapters) {
    if (c.part !== lastPart && !c.front) {
      toc += `<div class="toc-part">${PART_NAMES[c.part] || c.part}</div>`;
      body += `<section class="part-page"><h1>${PART_NAMES[c.part] || c.part}</h1></section>`;
    }
    lastPart = c.part;
    const id = `c-${c.slug}`;
    toc += `<div class="toc-item"><a href="#${id}">${c.num ? c.num + '. ' : ''}${c.title}</a></div>`;
    body += `<section class="chapter" id="${id}">${render(c)}</section>`;
  }
  toc += '</nav>';
  const cover = `<section class="cover"><h1>${meta.title}</h1><p class="cover-sub">${meta.subtitle}</p><p class="cover-author">${meta.author}</p></section>`;
  const html = `<!doctype html><html lang="vi"><head><meta charset="utf-8"><title>${BOOK_TITLE}</title><style>${css}</style></head><body>${cover}${toc}${body}</body></html>`;
  fs.mkdirSync(path.join(FORMATS, '_tmp'), { recursive: true });
  const htmlFile = path.join(FORMATS, '_tmp', 'book-print.html');
  fs.writeFileSync(htmlFile, html, 'utf8');
  const pdf = path.join(FORMATS, 'book.pdf');
  execFileSync(findChrome(), [
    '--headless=new', '--disable-gpu', '--no-pdf-header-footer',
    `--print-to-pdf=${pdf}`, '--virtual-time-budget=10000',
    'file:///' + htmlFile.replace(/\\/g, '/'),
  ], { stdio: 'ignore' });
  console.log(`PDF: ${path.relative(ROOT, pdf)} (${(fs.statSync(pdf).size / 1024).toFixed(0)} KB)`);
}

// ---------- kiem tra ----------
function check() {
  const chapters = loadChapters();
  const errors = [];
  let prev = 0;
  for (const c of chapters) {
    if (c.front) continue;
    if (!c.num) errors.push(`${c.rel}: thieu 'chapter:' trong front-matter`);
    else if (c.num !== prev + 1) errors.push(`${c.rel}: so chuong ${c.num} khong lien tuc (truoc do ${prev})`);
    if (c.num) prev = c.num;
    if (c.fm.code && !fs.existsSync(path.join(ROOT, c.fm.code))) errors.push(`${c.rel}: thu muc code '${c.fm.code}' khong ton tai`);
    for (const m of c.body.matchAll(/\{\{code:([^}]+)\}\}/g)) {
      if (!fs.existsSync(path.join(ROOT, 'code', m[1]))) errors.push(`${c.rel}: thieu code/${m[1]}`);
    }
    if (/^```\s*$/m.test(c.body.replace(/```[^\n]+\n[\s\S]*?\n```/g, ''))) errors.push(`${c.rel}: co code fence khong khai bao ngon ngu`);
  }
  if (errors.length) { console.error(errors.join('\n')); process.exit(1); }
  console.log(`Check OK: ${chapters.length} file.`);
}

const cmd = process.argv[2] || 'all';
if (cmd === 'check' || cmd === 'all') check();
if (cmd === 'html' || cmd === 'all') buildHtml();
if (cmd === 'pdf' || cmd === 'all') buildPdf();
if (!['check', 'html', 'pdf', 'all'].includes(cmd)) { console.error('Dung: node tools/build.js html|pdf|check|all'); process.exit(1); }
