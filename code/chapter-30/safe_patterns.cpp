// Cac mau ma cong cu phan tich tinh (clang-tidy, cppcheck) va cac chuan nhu MISRA C++/AUTOSAR/CERT
// thuong yeu cau, viet duoi dang "truoc/sau" de de doi chieu. Chi phan "sau" duoc bien dich (phan
// "truoc" chi la CHU THICH minh hoa loi, khong bien dich - vi no co canh bao/loi co y).
#include <cstdint>
#include <cstdio>

// --- 1) Moi bien phai duoc khoi tao ---
// TRUOC (khong bien dich trong vi du nay, chi minh hoa):
//   int result;               // chua khoi tao
//   if (input > 0) result = 1;
//   return result;            // duong nao input <= 0 thi result la rac
int classify_bad_style_fixed(int input) {
    int result = 0;  // SAU: luon co gia tri xac dinh, moi nhanh deu gan lai neu can
    if (input > 0) result = 1;
    else if (input < 0) result = -1;
    return result;
}

// --- 2) Tranh chuyen doi thu hep ngam dinh; dung static_cast tuong minh ---
std::uint8_t clamp_to_byte(int value) {
    if (value < 0) return 0;
    if (value > 255) return 255;
    return static_cast<std::uint8_t>(value);  // tuong minh: nguoi doc thay ro dang "cat" gia tri
}

// --- 3) Khong dung goto; dung return som hoac vong lap co dieu kien ro rang ---
// TRUOC (minh hoa, khong bien dich): dung goto de "thoat som" qua nhieu tang long nhau.
bool validate_frame(const std::uint8_t* data, std::size_t len) {
    if (data == nullptr) return false;   // return som thay cho goto loi
    if (len == 0) return false;
    if (len > 64) return false;
    return true;
}

// --- 4) Khong dung so "ma thuat"; dat ten hang so ro rang ---
constexpr int kMaxRetries = 3;
constexpr std::uint32_t kTimeoutMs = 500;

bool send_with_retry(int attempt) {
    if (attempt >= kMaxRetries) {
        std::printf("het %d lan thu, bao loi (timeout moi lan %u ms)\n", kMaxRetries,
                    static_cast<unsigned>(kTimeoutMs));
        return false;
    }
    return true;
}

int main() {
    std::printf("classify(5) = %d, classify(-5) = %d, classify(0) = %d\n",
                classify_bad_style_fixed(5), classify_bad_style_fixed(-5), classify_bad_style_fixed(0));

    std::printf("clamp_to_byte(300) = %u, clamp_to_byte(-10) = %u, clamp_to_byte(200) = %u\n",
                static_cast<unsigned>(clamp_to_byte(300)), static_cast<unsigned>(clamp_to_byte(-10)),
                static_cast<unsigned>(clamp_to_byte(200)));

    std::printf("validate_frame(null, 10) = %s\n", validate_frame(nullptr, 10) ? "hop le" : "khong hop le");

    for (int i = 0; i <= kMaxRetries; ++i) send_with_retry(i);
}
