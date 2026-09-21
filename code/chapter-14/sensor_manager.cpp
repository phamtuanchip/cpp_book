// Trình quản lý nhiều cảm biến qua một giao diện đa hình chung: mẫu thường gặp
// trong firmware có nhiều loại cảm biến cắm được (plug-in) và chọn lúc chạy.
#include <array>
#include <cstdint>
#include <cstdio>

class ISensor {
public:
    virtual ~ISensor() = default;           // BẮT BUỘC: xoá qua con trỏ base phải gọi đúng destructor con
    virtual bool init() = 0;
    [[nodiscard]] virtual int read() = 0;
    [[nodiscard]] virtual const char* name() const = 0;
};

class FakeTemp final : public ISensor {
public:
    explicit FakeTemp(int start) : value_(start) {}
    bool init() override { std::printf("[%s] init\n", name()); return true; }
    int read() override { return value_++; }
    const char* name() const override { return "FakeTemp"; }
private:
    int value_;
};

class FakeHumidity final : public ISensor {
public:
    bool init() override { std::printf("[%s] init\n", name()); return true; }
    int read() override { return 55; }
    const char* name() const override { return "FakeHumidity"; }
};

// Chỉ phụ thuộc giao diện ISensor: thêm cảm biến mới không cần sửa hàm này.
static void poll_all(std::array<ISensor*, 2>& sensors) {
    for (ISensor* s : sensors) std::printf("%-14s -> %d\n", s->name(), s->read());
}

int main() {
    FakeTemp temp(20);
    FakeHumidity humidity;
    std::array<ISensor*, 2> sensors{&temp, &humidity};

    for (ISensor* s : sensors) s->init();
    poll_all(sensors);
    poll_all(sensors);   // FakeTemp tăng dần mỗi lần đọc, FakeHumidity cố định
    return 0;
}
