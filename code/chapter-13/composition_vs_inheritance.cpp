// So sánh "has-a" (tổ hợp) và "is-a" (kế thừa) trên cùng một bài toán: một cảm biến
// nhiệt độ giao tiếp qua bus I2C.
#include <cstdint>
#include <cstdio>

// ---- Bus I2C giả lập: một thiết bị dùng CHUNG bus với nhiều thiết bị khác ----
class I2cBus {
public:
    explicit I2cBus(int id) : id_(id) {}
    [[nodiscard]] std::uint8_t read_register(std::uint8_t addr, std::uint8_t reg) const {
        std::printf("  I2C%d: doc dia chi 0x%02X, thanh ghi 0x%02X\n", id_, addr, reg);
        return 0x19;   // giá trị giả lập
    }
private:
    int id_;
};

// ---- SAI (thường gặp): kế thừa "is-a" trong khi quan hệ thật là "has-a" ----
// TempSensorBad "LÀ MỘT" I2cBus? Không hợp lý: cảm biến không phải là cái bus, nó CHỈ
// DÙNG bus. Kế thừa kiểu này còn lộ ra ngoài các hàm nội bộ của I2cBus không liên quan
// đến cảm biến, và không cho phép hai cảm biến chia sẻ MỘT bus vật lý (mỗi bản kế thừa
// tạo bus riêng).
class TempSensorBad : public I2cBus {
public:
    explicit TempSensorBad(int bus_id) : I2cBus(bus_id) {}
    [[nodiscard]] int read_celsius() const { return read_register(0x48, 0x00); }
};

// ---- ĐÚNG: tổ hợp "has-a" — cảm biến GIỮ MỘT THAM CHIẾU tới bus dùng chung ----
class TempSensor {
public:
    TempSensor(I2cBus& bus, std::uint8_t address) : bus_(bus), address_(address) {}
    [[nodiscard]] int read_celsius() const { return bus_.read_register(address_, 0x00); }
private:
    I2cBus& bus_;          // không sở hữu bus; bus sống lâu hơn cảm biến (do người gọi đảm bảo)
    std::uint8_t address_;
};

int main() {
    I2cBus bus(1);
    TempSensor temp(bus, 0x48);       // hai cảm biến CÙNG DÙNG một bus vật lý
    TempSensor humidity(bus, 0x40);

    std::printf("temp     = %d\n", temp.read_celsius());
    std::printf("humidity = %d\n", humidity.read_celsius());

    TempSensorBad bad(1);              // tạo "bus riêng" bên trong — không phản ánh phần cứng thật
    std::printf("bad      = %d\n", bad.read_celsius());
    return 0;
}
