// Uoc luong kich thuoc moi khung stack bang cach do dia chi cua bien cuc bo o cac do sau de quy khac nhau.
// Ky thuat "high-water mark" that (vd. FreeRTOS uxTaskGetStackHighWaterMark) dien vung stack bang mot
// mau (0xAA...) luc khoi dong, roi sau do quet tu day len de tim byte dau tien con nguyen mau -> phan
// stack chua bao gio dung toi. O day ta chi minh hoa PHAN "moi khung ton bao nhieu byte", don gian hoa
// de chay an toan tren host (khong dung mau/quet that vi khong kiem soat duoc vung stack tren host).
#include <cstddef>
#include <cstdint>
#include <cstdio>

std::uintptr_t addr_of_local() {
    volatile int probe = 0;  // volatile de trinh bien dich khong toi uu mat bien nay
    return reinterpret_cast<std::uintptr_t>(&probe);
}

// Ham de quy nong: moi lan goi them mot khung stack. Bien 'guard' chi de tao mot khung co kich thuoc dang ke.
std::uintptr_t recurse(int depth, int target, std::uintptr_t addrs[]) {
    volatile std::uint8_t guard[32] = {};
    (void)guard;
    if (depth == target) addrs[0] = addr_of_local();
    if (depth == 0) return addrs[0];
    return recurse(depth - 1, target, addrs);
}

int main() {
    constexpr int kDepth = 20;
    std::uintptr_t shallow = 0;
    std::uintptr_t deep = 0;

    std::uintptr_t tmp[1];
    recurse(kDepth, kDepth, tmp);
    shallow = tmp[0];
    recurse(kDepth, 0, tmp);
    deep = tmp[0];

    // Tren kien truc stack-grows-down (hau het CPU nhung/PC), dia chi giam dan khi de quy sau hon.
    const auto diff = shallow > deep ? shallow - deep : deep - shallow;
    std::printf("dia chi o do sau %d: 0x%08llX\n", kDepth, static_cast<unsigned long long>(shallow));
    std::printf("dia chi o do sau 0:  0x%08llX\n", static_cast<unsigned long long>(deep));
    std::printf("uoc luong ~%llu byte cho %d khung -> ~%llu byte/khung (con so that phu thuoc trinh bien dich/co -O)\n",
                static_cast<unsigned long long>(diff), kDepth, static_cast<unsigned long long>(diff / kDepth));

    std::puts("Ky thuat that tren MCU: dien 0xAA vao toan bo vung stack luc khoi dong (truoc khi goi ham nao),");
    std::puts("sau do dinh ky quet tu day stack len, dem so byte con nguyen 0xAA -> do la luong con lai chua dung toi.");
}
