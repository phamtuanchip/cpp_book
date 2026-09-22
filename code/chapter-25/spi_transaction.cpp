// SPI thuong can chon chip (chip-select, CS) TRUOC khi truyen va tha ra SAU khi xong, du dung hay loi.
// RAII (chuong 12) lam dieu nay tu dong: mot 'guard' keo CS xuong luc tao, keo len luc huy.
#include <array>
#include <cstdint>
#include <cstdio>
#include <span>

// Gia lap thanh ghi CS cua mot chan GPIO (xem chuong 23 de biet ban that dung GpioPin<...>).
static bool g_cs_active = false;

class ChipSelectGuard {
public:
    ChipSelectGuard() {
        g_cs_active = true;
        std::puts("CS: xuong (chon chip)");
    }
    ChipSelectGuard(const ChipSelectGuard&) = delete;
    ChipSelectGuard& operator=(const ChipSelectGuard&) = delete;
    ~ChipSelectGuard() {
        g_cs_active = false;
        std::puts("CS: len (tha chip)");
    }
};

// Gia lap phan cung SPI: moi byte gui ra, nhan ve byte "echo + 1" de de kiem tra trong vi du nay.
std::uint8_t simulated_spi_transfer_byte(std::uint8_t out) {
    return static_cast<std::uint8_t>(out + 1);
}

// API GIAO DICH (transaction): nhan mot vung du lieu gui va mot vung nhan, tu quan ly CS qua RAII.
// Neu ham thoat SOM (return giua chung, hoac sau nay them exception), CS van duoc tha dung luc.
void spi_transaction(std::span<const std::uint8_t> tx, std::span<std::uint8_t> rx) {
    ChipSelectGuard cs;  // CS xuong ngay tai day
    if (tx.size() != rx.size()) {
        std::puts("loi: kich thuoc tx/rx khac nhau, huy giao dich");
        return;  // CS van duoc tha dung khi 'cs' ra khoi pham vi ham
    }
    for (std::size_t i = 0; i < tx.size(); ++i) rx[i] = simulated_spi_transfer_byte(tx[i]);
}  // CS tha o day, sau moi duong thoat

int main() {
    const std::array<std::uint8_t, 4> tx{0x10, 0x20, 0x30, 0x40};
    std::array<std::uint8_t, 4> rx{};

    spi_transaction(tx, rx);
    std::printf("nhan duoc: ");
    for (auto b : rx) std::printf("0x%02X ", static_cast<unsigned>(b));
    std::printf("\n");
    std::printf("CS con dang chon chip sau giao dich? %s\n", g_cs_active ? "co (loi!)" : "khong (dung)");

    // Giao dich loi (kich thuoc lech) van tha CS dung cach.
    std::array<std::uint8_t, 2> rx_short{};
    spi_transaction(tx, rx_short);
    std::printf("CS sau giao dich loi? %s\n", g_cs_active ? "co (loi!)" : "khong (dung)");
}
