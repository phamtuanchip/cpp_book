// May trang thai huu han (FSM) chong doi phim (debounce) bang enum class + switch. Duoc goi dinh ky
// (vd tu timer 1ms), khong dung delay() chan - phu hop firmware thoi gian thuc.
#include <cstdint>
#include <cstdio>

enum class ButtonState : std::uint8_t { Released, Debouncing, Pressed, Releasing };

class Button {
public:
    // Goi ham nay MOI lan tick (vd moi 1ms) voi trang thai chan GPIO tho (chua loc).
    void tick(bool raw_pressed) {
        switch (state_) {
            case ButtonState::Released:
                if (raw_pressed) { state_ = ButtonState::Debouncing; counter_ = 0; }
                break;
            case ButtonState::Debouncing:
                if (!raw_pressed) { state_ = ButtonState::Released; break; }
                if (++counter_ >= kDebounceTicks) { state_ = ButtonState::Pressed; on_press_edge_ = true; }
                break;
            case ButtonState::Pressed:
                if (!raw_pressed) { state_ = ButtonState::Releasing; counter_ = 0; }
                break;
            case ButtonState::Releasing:
                if (raw_pressed) { state_ = ButtonState::Pressed; break; }
                if (++counter_ >= kDebounceTicks) { state_ = ButtonState::Released; on_release_edge_ = true; }
                break;
        }
    }

    // "Canh" (edge) chi dung MOT lan sau khi doc - tranh xu ly lap lai o vong lap chinh.
    bool consume_press_edge() { const bool v = on_press_edge_; on_press_edge_ = false; return v; }
    bool consume_release_edge() { const bool v = on_release_edge_; on_release_edge_ = false; return v; }
    bool is_pressed() const { return state_ == ButtonState::Pressed || state_ == ButtonState::Releasing; }

private:
    static constexpr std::uint8_t kDebounceTicks = 5;  // 5 tick (vd 5ms neu tick moi 1ms)
    ButtonState state_ = ButtonState::Released;
    std::uint8_t counter_ = 0;
    bool on_press_edge_ = false;
    bool on_release_edge_ = false;
};

int main() {
    Button btn;
    // Gia lap tin hieu GPIO tho, nhieu (rung) trong vai tick dau khi nhan/tha.
    const bool raw_signal[] = {
        false, false, true, false, true, true, true, true, true, true,  // nhan (rung roi on dinh)
        true, true, true, true, true,                                   // giu
        false, true, false, false, false, false, false, false,          // tha (rung roi on dinh)
        false, false,
    };

    for (std::size_t t = 0; t < sizeof(raw_signal) / sizeof(raw_signal[0]); ++t) {
        btn.tick(raw_signal[t]);
        if (btn.consume_press_edge()) std::printf("tick %2zu: PHAT HIEN NHAN (da loc rung)\n", t);
        if (btn.consume_release_edge()) std::printf("tick %2zu: PHAT HIEN THA (da loc rung)\n", t);
    }
}
