// HAL (Hardware Abstraction Layer) bang concept + template: driver cam bien khong biet no chay tren
// I2C that hay mot bus gia lap, mien la kieu do thoa man concept I2cBus. Giai quyet luc bien dich,
// khong vtable (so sanh voi HAL dung lop ao o chuong 14 khi can chon bus luc chay).
#include <array>
#include <concepts>
#include <cstdint>
#include <cstdio>

template <typename T>
concept I2cBus = requires(T bus, std::uint8_t addr, std::uint8_t reg, std::uint8_t value) {
    { bus.write_reg(addr, reg, value) } -> std::same_as<void>;
    { bus.read_reg(addr, reg) } -> std::same_as<std::uint8_t>;
};

// Bus gia lap tren host: mot mang thanh ghi don gian dai dien cho mot thiet bi I2C.
class FakeI2cBus {
public:
    void write_reg(std::uint8_t addr, std::uint8_t reg, std::uint8_t value) {
        std::printf("I2C ghi: addr=0x%02X reg=0x%02X value=0x%02X\n",
                    static_cast<unsigned>(addr), static_cast<unsigned>(reg), static_cast<unsigned>(value));
        if (addr == kSensorAddr && reg < regs_.size()) regs_[reg] = value;
    }
    std::uint8_t read_reg(std::uint8_t addr, std::uint8_t reg) {
        if (addr == kSensorAddr && reg < regs_.size()) return regs_[reg];
        return 0xFF;
    }

    static constexpr std::uint8_t kSensorAddr = 0x76;

private:
    std::array<std::uint8_t, 8> regs_{{0, 0, 0, 0, 25, 0, 0, 0}};  // regs_[4] gia lap nhiet do = 25
};

// Driver cam bien nhiet do: nhan BUS qua template, rang buoc boi concept I2cBus.
// Khong quan tam BUS la FakeI2cBus, hay mot driver I2C phan cung that - mien thoa man giao dien.
template <I2cBus Bus>
class TempSensor {
public:
    explicit TempSensor(Bus& bus) : bus_(bus) {}

    void init() { bus_.write_reg(kAddr, kCtrlReg, 0x01); }  // gia lap: bat che do do lien tuc
    std::uint8_t read_celsius() { return bus_.read_reg(kAddr, kTempReg); }

private:
    static constexpr std::uint8_t kAddr = FakeI2cBus::kSensorAddr;
    static constexpr std::uint8_t kCtrlReg = 0;
    static constexpr std::uint8_t kTempReg = 4;
    Bus& bus_;
};

int main() {
    FakeI2cBus bus;
    TempSensor sensor{bus};  // suy luan kieu tu tham so ham tao (CTAD, C++17)

    sensor.init();
    std::printf("nhiet do doc duoc = %u C\n", static_cast<unsigned>(sensor.read_celsius()));
}
