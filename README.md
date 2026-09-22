# Học lập trình C++ cho kỹ sư hệ thống nhúng — Từ cơ bản đến nâng cao

Dạy C++ từ cơ bản đến nâng cao, tập trung cho kỹ sư hệ thống nhúng: từ câu chuyện nền tảng, ngôn ngữ cấp thấp, thực tiễn sử dụng đến định hướng phát triển và các ngôn ngữ hỗ trợ mới (Rust, Zig...).

- Đọc online (GitHub Pages): https://phamtuanchip.github.io/cpp_book/
- Định dạng đầu ra: **HTML** và **PDF** trước; **EPUB** và **Amazon KDP** sau.
- Sách viết cùng AI: mỗi chương có prompt sinh nội dung, checklist duyệt, và ví dụ chạy được.

---

## 1. Độc giả mục tiêu

- Kỹ sư nhúng đang dùng C, muốn chuyển sang C++ hiện đại (C++17/20) mà không mất kiểm soát phần cứng.
- Sinh viên Điện tử / Cơ điện tử / CNTT học MCU, RTOS, embedded Linux.
- Lập trình viên C++ desktop muốn hiểu ràng buộc bộ nhớ, real-time, no-exceptions/no-RTTI.

## 2. Đề cương sách (dự kiến ~30 chương, 8 phần)

| Phần | Chủ đề | Chương chính |
|------|--------|--------------|
| 0 | Nền tảng & câu chuyện | Lịch sử máy tính, ngôn ngữ máy, Assembly, C → C++; vì sao nhúng vẫn cần C++; chi phí trừu tượng; **bản đồ ngành và định hướng nghề nghiệp** (ô tô, IoT, smart home, gia dụng, thiết bị đeo: học gì, làm dự án gì, kinh nghiệm thực chiến) |
| 1 | Môi trường & công cụ | g++/clang, arm-none-eabi-gcc, CMake, Compiler Explorer, QEMU/Renode, debug bằng gdb/OpenOCD |
| 2 | C++ cơ bản | Kiểu dữ liệu, biến, hàm, con trỏ/tham chiếu, `const`/`constexpr`, mảng, chuỗi |
| 3 | OOP cho nhúng | class, RAII, kế thừa, đa hình (chi phí vtable), template cơ bản |
| 4 | C++ hiện đại | `auto`, lambda, `std::array`, `std::span`, `std::optional`, smart pointer, move semantics, concepts |
| 5 | Bộ nhớ & cấp thấp | Memory layout, stack/heap, allocator tùy biến, `volatile`, memory-mapped I/O, bit-field, alignment, endianness |
| 6 | Nhúng thực chiến | GPIO/UART/SPI/I2C driver bằng C++, ngắt, HAL, state machine, RTOS (FreeRTOS/Zephyr), giao tiếp C ↔ C++ |
| 7 | Chất lượng & hiệu năng | Unit test (host-based), static analysis, MISRA C++/AUTOSAR/CERT, tối ưu code size & tốc độ, ETL |
| 8 | Định hướng | Rust, Zig, C++ safety profiles, xu hướng nhúng + AI (TinyML); phụ lục, cheat sheet, lỗi thường gặp |

Mỗi chương theo khung cố định: **Mục tiêu → Câu chuyện/động lực → Kiến thức → Ví dụ chạy được → Góc nhúng (chi phí RAM/Flash) → Lỗi thường gặp → Bài tập → Tóm tắt**.

## 3. Ngôn ngữ / công cụ chọn cho pipeline

Đã so sánh với các sách hiện có: `c_book` (Python + pandoc + KDP), `book_ai` (Node + Chrome headless PDF), `cshap_book`/`java_book`/`book_ba` (Markdown + Pages).

**Quyết định:**

| Lớp | Chọn | Lý do |
|-----|------|-------|
| Nguồn sách | **Markdown** (+ YAML front-matter) | AI sinh/sửa tốt nhất, diff sạch trong git, một nguồn cho mọi định dạng |
| Điều phối build | **Node.js** (`tools/build.js`, markdown-it + highlight.js) | Máy hiện tại có Node + Chrome nhưng chưa có Python/pandoc; cùng cách làm `book_ai`/`c_book/build-html.js`. Python + pandoc (`c_book/tools/bookgen`) vẫn là phương án cho EPUB ở G3 |
| Chuyển đổi | markdown-it → HTML; pandoc (G3) → EPUB3 | HTML/PDF cần ngay; EPUB chuẩn KDP làm sau bằng pandoc hoặc epub-gen |
| PDF | **Chrome/Edge headless** in từ HTML in-ấn (mặc định); lualatex là tùy chọn | Không cần cài TeX nặng, font tiếng Việt và CSS dễ; `book_ai` đã chứng minh chạy tốt |
| Kiểm chứng ví dụ | **CMake + g++ (host)** + Makefile theo chương, CI GitHub Actions | Code mẫu phải biên dịch và chạy được; lưu output vào `*.runout.txt` như `c_book` |
| Kiểm tra EPUB | **epubcheck** + **Kindle Previewer** | Bắt buộc trước khi upload KDP |
| Xuất bản | **Amazon KDP**, upload EPUB3 (hoặc KPF qua Kindle Create) | Tự phát hành, không cần trung gian |

> Đường build hiện hành là Node.js cho HTML/PDF. Python + pandoc chỉ thêm vào khi làm EPUB (G3).

## 4. Cấu trúc thư mục

Kế thừa từ `c_book` (manuscript/code/kdp/formats/tools), kết hợp chia phần như `cshap_book`/`java_book`.

```
cpp_book/
├── README.md                  # tài liệu này
├── LICENSE
├── manuscript/                # NGUỒN SÁCH (Markdown)
│   ├── 00-front-matter/       # bìa trong, lời tựa, cách dùng sách, mục lục
│   ├── part0-nen-tang/        # ch01-... .md
│   ├── part1-moi-truong/
│   ├── ...
│   └── part8-dinh-huong/      # + phụ lục
├── code/                      # VÍ DỤ CHẠY ĐƯỢC, theo chương
│   ├── chapter-01/
│   │   ├── hello.cpp
│   │   ├── hello.runout.txt   # output thực tế đã ghi lại
│   │   ├── CMakeLists.txt
│   │   └── README.md
│   └── common/                # header dùng chung (mock HAL, tiện ích test)
├── assets/                    # hình, sơ đồ (SVG ưu tiên), font
├── prompts/                   # PROMPT DÙNG CHO AI (xem mục 6)
│   ├── system.md              # giọng văn, thuật ngữ, quy ước sách
│   ├── chapter-template.md    # khung chương
│   └── ch01.prompt.md ...     # prompt riêng từng chương
├── templates/                 # template pandoc + CSS
│   ├── book.css               # dùng chung HTML/EPUB
│   ├── print.css              # riêng cho PDF (@page, số trang)
│   └── epub.css
├── tools/build.js             # Node: html | pdf | check | all (EPUB thêm ở G3)
├── metadata.yaml              # tiêu đề, tác giả, ngôn ngữ, ISBN, bản quyền
├── kdp/                       # metadata KDP, mô tả, từ khóa, checklist, bìa
├── formats/                   # ĐẦU RA (gitignore phần build): book.epub, book.pdf, cover.jpg
├── dist/                      # site HTML (GitHub Pages)
├── blog/                      # bài giới thiệu (medium-intro-en.md)
├── package.json               # markdown-it, highlight.js
└── .github/workflows/
    ├── ci.yml                 # biên dịch + chạy code/, kiểm tra link
    ├── pages.yml              # build HTML → deploy Pages
    └── release.yml            # tag vX.Y → build PDF+EPUB → GitHub Release
```

## 5. Ví dụ mẫu (bắt buộc cho mỗi chương)

Quy tắc: **không ví dụ nào được đưa vào sách nếu chưa biên dịch và chạy thành công**. Output thật được ghi vào `*.runout.txt` và chèn vào chương bằng script, không viết tay.

### 5.1 Ví dụ cơ bản — `code/chapter-01/hello.cpp`

```cpp
#include <iostream>

int main() {
    std::cout << "Hello, embedded C++!\n";
    return 0;
}
```

```bash
g++ -std=c++20 -Wall -Wextra -o hello hello.cpp && ./hello
# Hello, embedded C++!
```

### 5.2 Ví dụ nhúng (chạy trên host nhờ mock) — RAII cho GPIO

```cpp
#include <cstdint>
#include <cstdio>

// Trên MCU thật: con trỏ tới thanh ghi. Trên host: biến mô phỏng.
static volatile std::uint32_t fake_gpio_odr = 0;

template <std::uint32_t Pin>
class Led {
public:
    Led()  { fake_gpio_odr |=  (1u << Pin); }   // bật khi tạo
    ~Led() { fake_gpio_odr &= ~(1u << Pin); }   // tắt khi hủy (RAII)
};

int main() {
    {
        Led<5> led;
        std::printf("ODR = 0x%08X\n", static_cast<unsigned>(fake_gpio_odr));
    }
    std::printf("ODR = 0x%08X\n", static_cast<unsigned>(fake_gpio_odr));
}
// ODR = 0x00000020
// ODR = 0x00000000
```

### 5.3 Mẫu một chương Markdown (`manuscript/part2-co-ban/ch08-constexpr.md`)

````markdown
---
chapter: 8
title: "constexpr — chuyển việc tính toán sang lúc biên dịch"
part: 2
code: code/chapter-08
---

# Chương 8. constexpr

## Mục tiêu
- Hiểu `constexpr` giúp tiết kiệm Flash/CPU trên MCU thế nào.

## Ví dụ
```cpp
constexpr int kBaud = 115200;
constexpr int divisor(int clk) { return clk / kBaud; }
static_assert(divisor(16'000'000) == 138);
```

## Góc nhúng
Bảng tra sin 256 phần tử tính lúc biên dịch: 0 chu kỳ CPU lúc chạy.

## Lỗi thường gặp
## Bài tập
## Tóm tắt
````

## 6. Quy trình viết sách cùng AI

1. **Khóa nội quy**: `prompts/system.md` — giọng văn tiếng Việt, giữ thuật ngữ tiếng Anh (RAII, ISR...), chuẩn C++ (mặc định C++20), tiêu chuẩn code (`-Wall -Wextra`), không bịa số liệu/API.
2. **Sinh dàn ý chương** từ `chapter-template.md` → người duyệt.
3. **Sinh bản thảo** vào `manuscript/...` (AI viết, front-matter đầy đủ).
4. **Sinh code mẫu** vào `code/chapter-NN/` → `npm run verify` biên dịch + chạy + ghi `*.runout.txt`.
5. **Người duyệt** (checklist): đúng kỹ thuật, ví dụ chạy được, số liệu RAM/Flash có nguồn, không văn AI sáo rỗng, thuật ngữ nhất quán.
6. **Build** HTML/PDF, xem lại bố cục; sửa và lặp.
7. Chỉ khi cả sách qua duyệt mới làm EPUB/KDP.

Kiểm tra tự động (`tools/bookgen/checks.py`): code fence có ngôn ngữ, link nội bộ không gãy, ảnh có alt text, mọi `code:` trong front-matter tồn tại, thứ tự chương liên tục.

## 7. Lệnh build

```bash
npm install
npm run verify       # biên dịch kiểm tra mọi ví dụ (cần g++); thêm `-- --run` để chạy và ghi *.runout.txt
npm run check        # kiểm tra nguồn: số chương, code được chèn, fence có ngôn ngữ
npm run build:html   # → dist/ (GitHub Pages)
npm run build:pdf    # → formats/book.pdf (Chrome/Edge headless)
npm run build        # check + html + pdf
# Giai đoạn sau: epub, cover, package (KDP)
```

Yêu cầu: Node 18+, Chrome hoặc Edge (hoặc đặt `CHROME_PATH`), g++ + CMake để chạy ví dụ; cho EPUB/KDP thêm pandoc, epubcheck và Kindle Previewer.

## 8. Lộ trình

| Giai đoạn | Nội dung | Tiêu chí xong |
|-----------|----------|---------------|
| **G0. Khung** | Tạo cấu trúc thư mục, `metadata.yaml`, `prompts/`, CSS, bộ build từ `c_book`, CI, Pages | `generate_book.py html` chạy với 1 chương mẫu |
| **G1. Viết + HTML + PDF** | Viết phần 0–4 → 5–8, mỗi chương có code chạy được | Site HTML online; `book.pdf` đọc được, mục lục/số trang/font tiếng Việt đúng |
| **G2. Chất lượng** | Duyệt kỹ thuật, bài tập, phụ lục, cheat sheet | Checklist duyệt đạt cho mọi chương |
| **G3. EPUB** | Build EPUB3 (TOC, font nhúng, code không tràn dòng) | epubcheck 0 lỗi; xem tốt trên Kindle Previewer (Kindle, tablet, phone) |
| **G4. KDP** | Bìa (kích thước theo KDP), mô tả, 7 từ khóa, 2 danh mục, giá, ISBN (KDP cấp miễn phí) | Upload, xem bản proof, phát hành |
| **G5. Sau phát hành** | Bản in paperback (PDF trim-size + bleed), cập nhật phiên bản, phản hồi độc giả | Tag `vX.Y` tự động tạo release |

## 9. Ghi chú riêng cho Amazon KDP

- **EPUB**: dùng EPUB3 reflowable; code block dùng font monospace nhúng, tự xuống dòng; tránh bảng rộng (chuyển thành danh sách hoặc ảnh khi cần).
- **PDF không phải định dạng ebook chính** trên KDP; PDF chỉ dùng cho bản **paperback** (khổ 6x9 in hoặc 7x10 in, lề trong lớn hơn lề ngoài, font nhúng).
- **Bìa ebook**: JPG/TIFF, cạnh dài tối thiểu 2560 px, tỉ lệ 1.6:1.
- **Tiếng Việt**: khai báo `language: vi` trong metadata; kiểm tra font có đủ dấu.
- **Nội dung do AI hỗ trợ**: KDP yêu cầu khai báo khi nội dung do AI *tạo ra* (khác *hỗ trợ*); đọc chính sách hiện hành và khai báo đúng khi upload.
- **Bản quyền**: mã nguồn MIT; nội dung sách CC BY-NC như `c_book` hoặc All rights reserved nếu bán (quyết định trước khi phát hành, xem `kdp/checklist.md`).

## 10. Tiến độ

- [x] G0: khung thư mục, `tools/build.js` (html/pdf/check), CSS, `prompts/`, workflows Pages + CI.
- [x] Lời tựa + ch01–ch15 (Phần 0 ch01-04, Phần 1 ch05-06, Phần 2 ch07-11, Phần 3 ch12-15); build HTML/PDF chạy được.
- [x] g++ 16.2 (MSYS2 UCRT64) cài sẵn. `npm run verify` mặc định chỉ **biên dịch kiểm tra** (`-fsyntax-only`, nhanh, không tạo/chạy file thực thi — an toàn với phần mềm bảo mật máy); thêm `-- --run` khi cần chạy thật và sinh `*.runout.txt`. ch01–ch11 đã có output thật; ch12–ch15 mới biên dịch kiểm tra (theo yêu cầu tạm hoãn chạy exe), sách hiện hiển thị "chạy trên máy bạn để xem kết quả" ở các ví dụ đó — chạy `npm run verify -- --run chapter-12 chapter-13 chapter-14 chapter-15` để lấp đầy khi tiện.
- [ ] Tra cứu và xác minh lại các tên chuẩn/giao thức trong ch01, ch04 từ nguồn chính thức trước khi xuất bản.
- [x] g++ 16.2 (MSYS2 UCRT64) đã cài; 11 ví dụ biên dịch sạch với `-Wall -Wextra`, output thật lưu trong `code/chapter-*/*.runout.txt`; CMake ch05 chạy được. Các lỗi biên dịch sách nhắc tới (`Gpio<40>`, `enum class`→`int`) đã kiểm chứng.
- [ ] ch05 (môi trường cross/CMake toolchain file) và Phần 2 (C++ cơ bản).
- [x] Bật GitHub Pages cho repo (nguồn: GitHub Actions qua `pages.yml`); site đã deploy thành công.
- [x] Bản thảo đầy đủ ch16–ch36 (Phần 4 C++ hiện đại, Phần 5 bộ nhớ, Phần 6 nhúng thực chiến, Phần 7 chất lượng/hiệu năng, Phần 8 định hướng + phụ lục cheat sheet); `npm run check` OK (37 file), `npm run build:html` chạy được.
- [x] ch12–ch35: **mọi ví dụ đã được CI biên dịch (`g++ -std=c++20 -Wall -Wextra`) và chạy thật**; output lưu trong `code/chapter-*/*.runout.txt`. Máy tác giả bị Application Control chặn chạy `g++`, nên `ci.yml` đóng vai trò "máy biên dịch": nó sinh `*.runout.txt` và upload artifact tên `runout` để tải về commit (`gh run download <id> -n runout`).

Build: `npm install` rồi `npm run build` (hoặc `build:html`, `build:pdf`, `check`).

## Liên hệ & bản quyền

Tác giả: Lukas — phamtuanchip@gmail.com  
Code: MIT. Nội dung sách: xem mục 9.
