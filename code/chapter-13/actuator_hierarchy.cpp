// Kế thừa "is-a" hợp lý: Led và Buzzer THỰC SỰ LÀ một loại OutputDevice
// (cùng giao diện on()/off(), khác cách hiện thực).
#include <cstdint>
#include <cstdio>

class OutputDevice {
public:
    virtual ~OutputDevice() = default;      // destructor ảo: bắt buộc khi có kế thừa + xoá qua con trỏ base
    virtual void on() = 0;
    virtual void off() = 0;
    [[nodiscard]] bool is_on() const { return on_; }
protected:
    bool on_ = false;
};

class Led : public OutputDevice {
public:
    explicit Led(std::uint8_t pin) : pin_(pin) {}
    void on() override  { on_ = true;  std::printf("Led(pin=%u) BAT\n", static_cast<unsigned>(pin_)); }
    void off() override { on_ = false; std::printf("Led(pin=%u) TAT\n", static_cast<unsigned>(pin_)); }
private:
    std::uint8_t pin_;
};

class Buzzer : public OutputDevice {
public:
    explicit Buzzer(std::uint32_t freq_hz) : freq_hz_(freq_hz) {}
    void on() override  { on_ = true;  std::printf("Buzzer(%u Hz) KEU\n", static_cast<unsigned>(freq_hz_)); }
    void off() override { on_ = false; std::printf("Buzzer im lang\n"); }
private:
    std::uint32_t freq_hz_;
};

// Mã ứng dụng chỉ biết "OutputDevice", không quan tâm cụ thể là Led hay Buzzer.
static void alarm_sequence(OutputDevice& d) {
    d.on();
    d.off();
}

int main() {
    Led led(5);
    Buzzer buzzer(2000);

    OutputDevice* devices[] = {&led, &buzzer};   // một mảng chứa các loại thiết bị khác nhau
    for (OutputDevice* d : devices) alarm_sequence(*d);

    std::printf("led.is_on()=%d buzzer.is_on()=%d\n", led.is_on(), buzzer.is_on());
    return 0;
}
