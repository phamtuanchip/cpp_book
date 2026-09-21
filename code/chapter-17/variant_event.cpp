// std::variant + std::visit: tập sự kiện đóng, kiểm tra đủ nhánh lúc biên dịch, không heap, không vtable.
#include <array>
#include <cstdint>
#include <cstdio>
#include <variant>

struct ButtonPressed { std::uint8_t id; };
struct TimerExpired { std::uint32_t ms; };
struct RxByte { std::uint8_t value; };

using Event = std::variant<ButtonPressed, TimerExpired, RxByte>;

template <class... Ts>
struct Overloaded : Ts... { using Ts::operator()...; };
template <class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

void handle(const Event& e) {
    std::visit(Overloaded{
                   [](const ButtonPressed& b) { std::printf("nut %u nhan\n", static_cast<unsigned>(b.id)); },
                   [](const TimerExpired& t) { std::printf("timer het %u ms\n", static_cast<unsigned>(t.ms)); },
                   [](const RxByte& r) { std::printf("nhan byte 0x%02X\n", static_cast<unsigned>(r.value)); },
               },
               e);
}

int main() {
    const std::array<Event, 3> queue{ButtonPressed{2}, TimerExpired{100}, RxByte{0x41}};
    for (const auto& e : queue) handle(e);
    std::printf("sizeof(Event)=%zu\n", sizeof(Event));
}
