// enum class cho mã lỗi, [[nodiscard]] để không bỏ quên kết quả, nạp chồng hàm, đối số mặc định.
#include <cstdint>
#include <cstdio>

enum class Status : std::uint8_t { Ok, Timeout, Busy, InvalidArg };

constexpr const char* to_string(Status s) {
    switch (s) {
        case Status::Ok:         return "Ok";
        case Status::Timeout:    return "Timeout";
        case Status::Busy:       return "Busy";
        case Status::InvalidArg: return "InvalidArg";
    }
    return "?";
}

// Nạp chồng: cùng tên, khác kiểu đối số -> trình biên dịch chọn lúc biên dịch.
void log(std::uint32_t value)     { std::printf("[u32] %u\n", static_cast<unsigned>(value)); }
void log(const char* text)        { std::printf("[str] %s\n", text); }

// Đối số mặc định: gọi ngắn gọn cho trường hợp thường gặp.
[[nodiscard]] Status uart_send(const std::uint8_t* data, std::uint32_t len, std::uint32_t timeout_ms = 100) {
    if (data == nullptr || len == 0) return Status::InvalidArg;
    if (timeout_ms == 0) return Status::Timeout;
    return Status::Ok;
}

int main() {
    const std::uint8_t msg[] = {'o', 'k'};
    log(42u);
    log("khoi dong");

    Status s = uart_send(msg, 2);              // dùng timeout mặc định
    std::printf("gui binh thuong : %s\n", to_string(s));

    s = uart_send(nullptr, 2);
    std::printf("gui con tro rong: %s\n", to_string(s));

    s = uart_send(msg, 2, 0);
    std::printf("timeout = 0     : %s\n", to_string(s));

    // uart_send(msg, 2);   // bỏ comment: cảnh báo 'nodiscard' vì bỏ quên Status
    return 0;
}
