// "Phinh Flash do template" (template bloat): moi to hop tham so template khac nhau sinh ra MOT BAN
// MA RIENG. Neu logic khong thuc su phu thuoc tham so do (o day la N - kich thuoc mang), tach phan
// khong phu thuoc ra mot ham KHONG phai template de dung chung, chi giu phan thuc su can template mong.
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>

// ---- Cach de "phinh": moi N khac nhau la mot ham rieng, dai ---
template <typename T, std::size_t N>
T sum_bloat(const std::array<T, N>& a) {
    T total = T{};
    for (std::size_t i = 0; i < N; ++i) {
        total += a[i];
        if (a[i] > total) total = a[i];  // gia lap them vai dong logic de "ham" co than dang ke
    }
    return total;
}

// ---- Cach gon: phan logic (khong thuc su can biet N luc bien dich) tach thanh MOT ham chung theo T ----
template <typename T>
T sum_shared_impl(const T* data, std::size_t n) {
    T total = T{};
    for (std::size_t i = 0; i < n; ++i) {
        total += data[i];
        if (data[i] > total) total = data[i];
    }
    return total;
}

// Wrapper template MONG: chi chuyen std::array thanh (con tro, do dai), khong nhan ban logic cho tung N.
template <typename T, std::size_t N>
T sum_shared(const std::array<T, N>& a) {
    return sum_shared_impl<T>(a.data(), N);
}

int main() {
    const std::array<int, 4> a4{1, 5, 2, 8};
    const std::array<int, 10> a10{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    const std::array<int, 3> a3{100, 1, 1};

    // Voi sum_bloat: ba loi goi nay (N=4, N=10, N=3) sinh BA THAN HAM rieng (cung kieu int, khac N).
    std::printf("sum_bloat:  %d %d %d\n", sum_bloat(a4), sum_bloat(a10), sum_bloat(a3));

    // Voi sum_shared: ca ba loi goi CUNG dung chung MOT than ham sum_shared_impl<int>; phan template
    // theo N chi con lai mot dong "goi ham chung", gan nhu khong ton them Flash cho moi N moi.
    std::printf("sum_shared: %d %d %d\n", sum_shared(a4), sum_shared(a10), sum_shared(a3));

    std::puts("Kiem chung that: bien dich voi -Os, dung 'nm --size-sort' hoac Compiler Explorer de");
    std::puts("dem so ban ma sinh ra cho sum_bloat<int,4/10/3> so voi sum_shared_impl<int> (chi 1 ban).");
}
