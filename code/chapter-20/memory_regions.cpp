// Minh hoa cac vung bo nho: .text (ma lenh), .rodata (hang so), .data (bien tinh da khoi tao khac 0),
// .bss (bien tinh khoi tao 0), stack (bien cuc bo), heap (cap phat dong). Dia chi thuc te khac nhau
// giua cac lan chay/may (ASLR, linker) - quan trong la THU TU va VUNG, khong phai gia tri tuyet doi.
#include <cstdint>
#include <cstdio>

static const char kMessage[] = "hang so trong .rodata";     // .rodata (thuong duoc gop, co the o Flash tren MCU)
static std::uint32_t g_initialized = 0x1234;                 // .data (co gia tri khac 0 -> phai luu trong Flash + sao chep vao RAM luc khoi dong)
static std::uint32_t g_zero_filled;                           // .bss (khong ton dung luong trong Flash, chi ton RAM, duoc dat 0 luc khoi dong)

void show_function_address() {}                               // dai dien cho .text

std::uintptr_t addr_of(const void* p) { return reinterpret_cast<std::uintptr_t>(p); }

int main() {
    int local_var = 42;                                        // stack
    auto* heap_var = new std::uint32_t{99};                     // heap (tren MCU nhung, nhieu du an KHONG dung heap)

    std::printf(".text   (ham)         ~ 0x%08llX\n", static_cast<unsigned long long>(addr_of(reinterpret_cast<void*>(&show_function_address))));
    std::printf(".rodata (hang so)     ~ 0x%08llX\n", static_cast<unsigned long long>(addr_of(kMessage)));
    std::printf(".data   (g_initialized) ~ 0x%08llX\n", static_cast<unsigned long long>(addr_of(&g_initialized)));
    std::printf(".bss    (g_zero_filled) ~ 0x%08llX\n", static_cast<unsigned long long>(addr_of(&g_zero_filled)));
    std::printf("stack   (local_var)   ~ 0x%08llX\n", static_cast<unsigned long long>(addr_of(&local_var)));
    std::printf("heap    (heap_var)    ~ 0x%08llX\n", static_cast<unsigned long long>(addr_of(heap_var)));

    std::printf("g_zero_filled truoc khi gan = %u (duoc dat 0 tu dau, khong ton Flash de luu gia tri nay)\n",
                static_cast<unsigned>(g_zero_filled));

    delete heap_var;
}
