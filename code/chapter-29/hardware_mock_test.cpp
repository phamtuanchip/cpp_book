// Kiem thu mot driver PHU THUOC phan cung bang cach "tiem" (inject) mot bus GIA LAP thay vi bus that -
// ket hop HAL kieu concept/template cua chuong 25 voi kiem thu host-based. Khong can MCU nao ca.
#include <array>
#include <concepts>
#include <cstdint>
#include <cstdio>

template <typename T>
concept I2cBus = requires(T bus, std::uint8_t addr, std::uint8_t reg, std::uint8_t value) {
    { bus.write_reg(addr, reg, value) } -> std::same_as<void>;
    { bus.read_reg(addr, reg) } -> std::same_as<std::uint8_t>;
};

// Driver that su (giong tinh than chuong 25): doc nhiet do, bao "qua nhiet" neu vuot nguong.
template <I2cBus Bus>
class ThermalGuard {
public:
    ThermalGuard(Bus& bus, std::uint8_t addr, std::uint8_t temp_reg, std::uint8_t limit)
        : bus_(bus), addr_(addr), temp_reg_(temp_reg), limit_(limit) {}

    bool is_overheating() { return bus_.read_reg(addr_, temp_reg_) > limit_; }

private:
    Bus& bus_;
    std::uint8_t addr_, temp_reg_, limit_;
};

// Bus GIA LAP cho kiem thu: khong noi voi phan cung, chi tra ve gia tri da lap trinh san va
// GHI LAI cac loi goi de bai test kiem tra ("da goi dung dia chi/thanh ghi chua?").
class MockBus {
public:
    void write_reg(std::uint8_t addr, std::uint8_t reg, std::uint8_t value) {
        last_write_addr_ = addr;
        last_write_reg_ = reg;
        last_write_value_ = value;
        ++write_count_;
    }
    std::uint8_t read_reg(std::uint8_t addr, std::uint8_t reg) {
        last_read_addr_ = addr;
        last_read_reg_ = reg;
        ++read_count_;
        return programmed_value_;
    }

    void program_read_value(std::uint8_t v) { programmed_value_ = v; }
    int read_count() const { return read_count_; }
    std::uint8_t last_read_addr() const { return last_read_addr_; }

private:
    std::uint8_t programmed_value_ = 0;
    std::uint8_t last_write_addr_ = 0, last_write_reg_ = 0, last_write_value_ = 0;
    std::uint8_t last_read_addr_ = 0, last_read_reg_ = 0;
    int write_count_ = 0, read_count_ = 0;
};

static int g_pass = 0, g_fail = 0;
#define CHECK(cond)                                                                    \
    do {                                                                               \
        if (cond) { ++g_pass; }                                                        \
        else { ++g_fail; std::printf("  THAT BAI: %s (dong %d)\n", #cond, __LINE__); } \
    } while (0)

int main() {
    MockBus bus;
    ThermalGuard guard(bus, 0x50, 0x01, /*limit=*/80);  // CTAD: suy ra ThermalGuard<MockBus>

    bus.program_read_value(50);
    CHECK(!guard.is_overheating());  // 50 <= 80: binh thuong

    bus.program_read_value(95);
    CHECK(guard.is_overheating());   // 95 > 80: qua nhiet

    // Kiem tra driver da doc DUNG dia chi thiet bi, khong doc nham dia chi khac tren cung bus.
    CHECK(bus.last_read_addr() == 0x50);
    CHECK(bus.read_count() == 2);

    std::printf("KET QUA: %d dat, %d that bai\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
