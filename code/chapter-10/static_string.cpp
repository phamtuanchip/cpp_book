// Chuỗi dung lượng cố định, không cấp phát động: thay cho std::string trên MCU.
#include <cstddef>
#include <cstdio>
#include <string_view>

template <std::size_t Capacity>
class StaticString {
public:
    // Ghi đè bằng nội dung định dạng; cắt bớt nếu quá dài. Trả về false nếu bị cắt.
    template <typename... Args>
    bool format(const char* fmt, Args... args) {
        int n = std::snprintf(buf_, sizeof buf_, fmt, args...);
        return n >= 0 && static_cast<std::size_t>(n) < sizeof buf_;
    }
    [[nodiscard]] std::string_view view() const { return std::string_view(buf_); }
    [[nodiscard]] const char* c_str() const { return buf_; }
    static constexpr std::size_t capacity() { return Capacity; }

private:
    char buf_[Capacity + 1] = {};   // +1 cho ký tự kết thúc
};

int main() {
    StaticString<24> line;
    bool ok = line.format("T=%d.%d C node=%u", 25, 3, 7u);
    std::printf("[%s] ok=%d len=%zu\n", line.c_str(), ok, line.view().size());

    StaticString<10> tiny;
    ok = tiny.format("qua dai: %d", 123456);   // bị cắt
    std::printf("[%s] ok=%d\n", tiny.c_str(), ok);

    std::string_view v = line.view().substr(0, 7);   // không sao chép
    std::printf("view=[%.*s]\n", static_cast<int>(v.size()), v.data());
    return 0;
}
