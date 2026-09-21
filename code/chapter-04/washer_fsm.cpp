// Máy trạng thái của một máy giặt (mô phỏng). Cùng mẫu này dùng cho lò nướng,
// máy lọc không khí, khoá thông minh... Chạy được trên PC, không cần phần cứng.
#include <array>
#include <cstdio>

enum class State { Idle, Filling, Washing, Draining, Done, Fault };
enum class Event { Start, WaterFull, TimerElapsed, Emptied, DoorOpened };

constexpr State next(State s, Event e) {
    switch (s) {
        case State::Idle:
            if (e == Event::Start) return State::Filling;
            break;
        case State::Filling:
            if (e == Event::WaterFull) return State::Washing;
            if (e == Event::DoorOpened) return State::Fault;   // an toàn: cửa mở khi đang cấp nước
            break;
        case State::Washing:
            if (e == Event::TimerElapsed) return State::Draining;
            if (e == Event::DoorOpened) return State::Fault;
            break;
        case State::Draining:
            if (e == Event::Emptied) return State::Done;
            break;
        case State::Done:
        case State::Fault:
            break;
    }
    return s;   // sự kiện không liên quan ở trạng thái này: giữ nguyên
}

constexpr const char* name(State s) {
    switch (s) {
        case State::Idle:     return "Idle";
        case State::Filling:  return "Filling";
        case State::Washing:  return "Washing";
        case State::Draining: return "Draining";
        case State::Done:     return "Done";
        case State::Fault:    return "Fault";
    }
    return "?";
}

// Kiểm tra quy tắc an toàn ngay lúc biên dịch.
static_assert(next(State::Idle, Event::Start) == State::Filling);
static_assert(next(State::Washing, Event::DoorOpened) == State::Fault);
static_assert(next(State::Done, Event::Start) == State::Done);

int main() {
    const std::array<Event, 4> normal{Event::Start, Event::WaterFull, Event::TimerElapsed, Event::Emptied};
    State s = State::Idle;
    std::puts("Chu trinh binh thuong:");
    for (Event e : normal) {
        s = next(s, e);
        std::printf("  -> %s\n", name(s));
    }

    const std::array<Event, 3> door_open{Event::Start, Event::WaterFull, Event::DoorOpened};
    s = State::Idle;
    std::puts("Mo cua giua chu trinh:");
    for (Event e : door_open) {
        s = next(s, e);
        std::printf("  -> %s\n", name(s));
    }
    return 0;
}
