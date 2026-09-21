// Bộ đếm mili-giây 32 bit tràn sau khoảng 49,7 ngày. Phép trừ không dấu vẫn cho khoảng thời gian đúng.
#include <cstdint>
#include <cstdio>
#include <initializer_list>

using Tick = std::uint32_t;

constexpr Tick elapsed(Tick now, Tick start) { return now - start; }   // đúng cả khi 'now' đã tràn

constexpr bool timed_out_wrong(Tick now, Tick deadline) { return now >= deadline; }            // sai khi tràn
constexpr bool timed_out(Tick now, Tick start, Tick timeout) { return elapsed(now, start) >= timeout; }

int main() {
    const Tick start = 0xFFFFFF00u;        // sắp tràn
    const Tick timeout = 1000;             // 1 giây
    const Tick deadline = start + timeout; // sau khi tràn còn nhỏ hơn start

    for (Tick offset : {Tick{100}, Tick{999}, Tick{1000}}) {
        Tick now = start + offset;
        std::printf("sau %4u ms: dung=%d sai=%d\n", static_cast<unsigned>(offset),
                    timed_out(now, start, timeout), timed_out_wrong(now, deadline));
    }
    return 0;
}
