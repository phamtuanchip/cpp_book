// Mau lien ket ISR (C linkage, ten ham co dinh do startup file/vector table doi hoi) voi driver viet
// bang C++. Tren MCU that, vector ngat trong startup.s tro toi dung TEN ham nay (vd "USART1_IRQHandler"),
// nen ham do PHAI co C linkage (extern "C") de ten khong bi bien doi (name mangling - xem chuong 28).
#include <cstdint>
#include <cstdio>

class UartDriver {
public:
    void on_rx_isr(std::uint8_t byte) {
        ++rx_count_;
        last_byte_ = byte;
    }
    std::uint32_t rx_count() const { return rx_count_; }
    std::uint8_t last_byte() const { return last_byte_; }

private:
    std::uint32_t rx_count_ = 0;
    std::uint8_t last_byte_ = 0;
};

// Doi tuong driver: static o pham vi file, song suot chuong trinh - ISR can mot noi co dinh de goi toi,
// khong the tao doi tuong "cuc bo" cho moi lan ngat.
static UartDriver g_uart1;

// C linkage: ten "USART1_IRQHandler" giu nguyen (khong bi C++ doi ten), khop voi ten startup file mong doi.
// Than ham la C++, duoc phep goi phuong thuc cua doi tuong C++ binh thuong.
extern "C" void USART1_IRQHandler() {
    // Tren phan cung that: doc thanh ghi DR/RDR cua USART1 de lay byte va xoa co ngat.
    // O day gia lap bang mot bien duoc "phan cung" ghi san truoc khi goi ham nay.
    static std::uint8_t simulated_hw_byte = 0;
    g_uart1.on_rx_isr(simulated_hw_byte++);
}

int main() {
    // Gia lap 3 lan ngat lien tiep (nhu the phan cung goi USART1_IRQHandler 3 lan).
    USART1_IRQHandler();
    USART1_IRQHandler();
    USART1_IRQHandler();

    std::printf("so byte da nhan = %u, byte cuoi = %u\n",
                static_cast<unsigned>(g_uart1.rx_count()), static_cast<unsigned>(g_uart1.last_byte()));
    std::puts("Luu y: day la mo phong tren host. Tren MCU that, ten ham ISR phai dung CHINH XAC");
    std::puts("ten ma vector ngat cua startup file/CMSIS mong doi - kiem tra tai lieu vendor SDK.");
}
