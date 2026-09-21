// Ba cách biểu diễn "một cảm biến": con trỏ hàm (C), lớp ảo (C++), template (C++).
#include <cstdio>

// ---- Cách 1 (C): struct chứa con trỏ hàm + con trỏ ngữ cảnh ----
struct SensorC {
    int (*read)(void* self);
    void* self;
};
struct FakeTempState { int celsius; };
static int fake_temp_read(void* self) { return static_cast<FakeTempState*>(self)->celsius; }

// ---- Cách 2 (C++): giao diện trừu tượng, đa hình lúc chạy (có vptr) ----
class ISensor {
public:
    virtual ~ISensor() = default;
    virtual int read() = 0;
};
class FakeTemp : public ISensor {
public:
    explicit FakeTemp(int c) : celsius_(c) {}
    int read() override { return celsius_; }
private:
    int celsius_;
};

// ---- Cách 3 (C++): template, đa hình lúc biên dịch (không vptr, gọi được inline) ----
class FakeTempStatic {
public:
    explicit FakeTempStatic(int c) : celsius_(c) {}
    int read() { return celsius_; }
private:
    int celsius_;
};

template <typename Sensor>
int average_of_two(Sensor& s) {
    return (s.read() + s.read()) / 2;
}

int main() {
    FakeTempState st{25};
    SensorC c{fake_temp_read, &st};
    std::printf("con tro ham (C) : %d\n", c.read(c.self));

    FakeTemp t{26};
    ISensor& i = t;
    std::printf("lop ao (C++)    : %d\n", i.read());

    FakeTempStatic ts{27};
    std::printf("template (C++)  : %d\n", average_of_two(ts));
    return 0;
}
