// std::pmr (polymorphic memory resource, C++17): dung mot vung nho TINH (static/stack) lam noi cap
// phat cho container chuan, khong goi malloc/new tren heap he thong.
#include <array>
#include <cstdio>
#include <memory_resource>
#include <vector>

int main() {
    // Vung nho co dinh, dat tren stack cua ham nay (co the dat 'static' de nam trong .bss thay vi stack).
    std::array<std::byte, 256> arena{};

    // monotonic_buffer_resource: cap phat tuan tu tu 'arena', KHONG giai phong tung phan tu rieng le
    // (chi 'giai phong' toan bo khi resource bi huy) - phu hop voi vong doi ngan, cap phat mot lan.
    std::pmr::monotonic_buffer_resource pool{arena.data(), arena.size(), std::pmr::null_memory_resource()};
    // null_memory_resource() lam "upstream": neu 'arena' het cho, cap phat them se that bai (bad_alloc)
    // thay vi am tham roi vao heap he thong - giup phat hien ngay luc dev neu vung tinh qua nho.

    std::pmr::vector<int> samples{&pool};
    samples.reserve(8);
    for (int i = 0; i < 8; ++i) samples.push_back(i * 10);

    std::printf("samples: ");
    for (int v : samples) std::printf("%d ", v);
    std::printf("\n");

    // monotonic_buffer_resource khong cong khai API "con lai bao nhieu byte"; muon biet chinh xac,
    // tu viet mot memory_resource bao ngoai de dem so byte da cap phat (bai tap cuoi chuong).
    std::printf("vung tinh: %zu byte; vector dang dung it nhat %zu byte cho du lieu phan tu\n",
                arena.size(), samples.capacity() * sizeof(int));
}
