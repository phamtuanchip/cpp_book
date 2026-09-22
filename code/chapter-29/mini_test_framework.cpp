// Mot bo test toi gian (khong can thu vien ngoai) de kiem thu logic THUAN (khong phan cung) tren host.
// Trong du an that, Catch2/doctest/GoogleTest lam viec nay tot hon nhieu (bao cao dep, nhieu tien ich) -
// xem phan van ban ve cach them qua CMake FetchContent. O day tu viet de vi du KHONG can gi ngoai g++.
#include <cstdio>
#include <string_view>

static int g_pass = 0;
static int g_fail = 0;

#define CHECK(cond)                                                                    \
    do {                                                                               \
        if (cond) { ++g_pass; }                                                        \
        else { ++g_fail; std::printf("  THAT BAI: %s (dong %d)\n", #cond, __LINE__); } \
    } while (0)

void run_test(std::string_view name, void (*fn)()) {
    std::printf("test: %.*s\n", static_cast<int>(name.size()), name.data());
    fn();
}

// ---- Doi tuong can kiem thu: mot FSM chong rung don gian, KHONG dung GPIO that (nhac lai chuong 26) ----
enum class ButtonState { Released, Debouncing, Pressed };

class Button {
public:
    bool tick(bool raw_pressed) {
        bool edge = false;
        switch (state_) {
            case ButtonState::Released:
                // Dem tu 1: chinh tick nay DA la tick dau tien thay tin hieu nhan.
                if (raw_pressed) { state_ = ButtonState::Debouncing; counter_ = 1; }
                break;
            case ButtonState::Debouncing:
                if (!raw_pressed) { state_ = ButtonState::Released; break; }
                if (++counter_ >= 3) { state_ = ButtonState::Pressed; edge = true; }
                break;
            case ButtonState::Pressed:
                if (!raw_pressed) state_ = ButtonState::Released;
                break;
        }
        return edge;
    }
    bool is_pressed() const { return state_ == ButtonState::Pressed; }

private:
    ButtonState state_ = ButtonState::Released;
    int counter_ = 0;
};

// ---- Cac ca kiem thu: moi ham la MOT kich ban, doc lap, khong phu thuoc thu tu chay ----
void test_button_khong_bao_gio_nhan_thi_khong_bao_gio_pressed() {
    Button b;
    for (int i = 0; i < 10; ++i) CHECK(!b.tick(false));
    CHECK(!b.is_pressed());
}

void test_button_nhan_du_lau_thi_chuyen_sang_pressed() {
    Button b;
    CHECK(!b.tick(true));   // tick 1: chua du debounce
    CHECK(!b.tick(true));   // tick 2
    CHECK(b.tick(true));    // tick 3: canh "vua nhan" xuat hien dung 1 lan
    CHECK(b.is_pressed());
    CHECK(!b.tick(true));   // giu nguyen: khong co canh moi
}

void test_button_rung_ngan_khong_tinh_la_nhan() {
    Button b;
    CHECK(!b.tick(true));
    CHECK(!b.tick(false));  // rung: nha ra truoc khi du debounce
    CHECK(!b.is_pressed());
}

int main() {
    run_test("khong nhan -> khong bao gio pressed", test_button_khong_bao_gio_nhan_thi_khong_bao_gio_pressed);
    run_test("nhan du lau -> chuyen sang pressed dung mot lan", test_button_nhan_du_lau_thi_chuyen_sang_pressed);
    run_test("rung ngan -> khong tinh la nhan", test_button_rung_ngan_khong_tinh_la_nhan);

    std::printf("\nKET QUA: %d dat, %d that bai\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
