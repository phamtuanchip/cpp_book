// So sanh 'volatile' don gian (co ban dung duoc cho co mot-nguoi-ghi/mot-nguoi-doc) voi std::atomic
// (dam bao them ve tinh nguyen tu va rao chan bo nho, can khi co doc-sua-ghi hoac nhieu loi/nhieu luong).
#include <atomic>
#include <cstdint>
#include <cstdio>

// Co bao "co du lieu moi" - chi MOT noi ghi (ISR gia lap), MOT noi doc (vong lap chinh).
// volatile la du: dam bao trinh bien dich khong cache gia tri trong thanh ghi CPU, luon doc/ghi that.
static volatile bool g_data_ready = false;
static volatile std::uint16_t g_last_sample = 0;

// Gia lap ISR: goi tu "phan cung" bat cu luc nao, khong dong bo voi vong lap chinh.
void simulated_isr(std::uint16_t sample) {
    g_last_sample = sample;
    g_data_ready = true;  // co dat SAU khi du lieu da san sang: thu tu nay quan trong
}

// Bo dem chia se, CAN atomic vi ca ISR lan vong lap chinh cung TANG no (doc-sua-ghi, khong an toan
// voi volatile don thuan neu chay tren loi khac hoac bi ngat giua chung tren cung mot loi).
static std::atomic<std::uint32_t> g_irq_count{0};

void simulated_isr_counting() {
    g_irq_count.fetch_add(1, std::memory_order_relaxed);
}

int main() {
    std::puts("-- volatile: mot noi ghi, mot noi doc --");
    simulated_isr(2048);
    if (g_data_ready) {
        std::printf("nhan mau moi: %u\n", static_cast<unsigned>(g_last_sample));
        g_data_ready = false;
    }

    std::puts("-- atomic: doc-sua-ghi an toan hon khi co tranh chap --");
    for (int i = 0; i < 5; ++i) simulated_isr_counting();
    std::printf("so lan ngat da dem = %u\n", static_cast<unsigned>(g_irq_count.load()));

    std::printf("sizeof(atomic<uint32_t>) = %zu (thuong bang sizeof(uint32_t), kiem tra .is_lock_free() tren toolchain that)\n",
                sizeof(g_irq_count));
    std::printf("atomic co lock-free tren kieu nay? %s\n", g_irq_count.is_lock_free() ? "co" : "khong (co the can khoa)");
}
