// Giao tiep C <-> C++: extern "C" tat name mangling, cho phep C goi ham C++ va nguoc lai; struct
// kieu C (POD, khong constructor/destructor/ham ao) co bo tri bo nho giong het ca hai phia.
#include <cstdint>
#include <cstdio>

// ---- 1) Mo phong mot "thu vien C cu" (vd vendor SDK viet bang C) ----
// Trong header C that, struct nay khong co gi ngoai du lieu tho - hoan toan hop le voi ca C lan C++.
extern "C" {
struct CSensorConfig {
    std::uint16_t sample_rate_hz;
    std::uint8_t gain;
};

// Kieu con tro ham theo phong cach C: callback nhan void* context (khong co lambda/std::function ben phia C).
using CReadyCallback = void (*)(void* ctx, std::int16_t value);

void c_sensor_init(const CSensorConfig* cfg);
void c_sensor_set_callback(CReadyCallback cb, void* ctx);
void c_sensor_poll();  // gia lap: "phan cung" tao du lieu moi va goi callback
}

// Dinh nghia (trong du an that, day la .c bien dich rieng hoac thu vien .a cua vendor - o day gop
// vao cung file .cpp de vi du tu chua, nhung van giu C linkage qua khoi extern "C" o tren).
static CReadyCallback g_callback = nullptr;
static void* g_callback_ctx = nullptr;
static CSensorConfig g_config{};

extern "C" void c_sensor_init(const CSensorConfig* cfg) {
    g_config = *cfg;
    std::printf("c_sensor_init: sample_rate=%u gain=%u\n",
                static_cast<unsigned>(cfg->sample_rate_hz), static_cast<unsigned>(cfg->gain));
}

extern "C" void c_sensor_set_callback(CReadyCallback cb, void* ctx) {
    g_callback = cb;
    g_callback_ctx = ctx;
}

extern "C" void c_sensor_poll() {
    if (g_callback) g_callback(g_callback_ctx, 1234);
}

// ---- 2) Boc thu vien C bang mot lop C++ RAII (nguoc lai: C++ goi API C) ----
class SensorHandle {
public:
    explicit SensorHandle(std::uint16_t rate, std::uint8_t gain) {
        CSensorConfig cfg{rate, gain};
        c_sensor_init(&cfg);
        c_sensor_set_callback(&SensorHandle::trampoline, this);  // ham C++ "tinh" lam cau noi
    }

    void poll() { c_sensor_poll(); }

    std::int16_t last_value() const { return last_value_; }

private:
    // "Trampoline": mot ham TINH (khong phai phuong thuc thuong) co the dung lam con tro ham kieu C.
    // Phuong thuc THUONG (khong static) khong the chuyen thanh con tro ham C vi no can them 'this' an.
    static void trampoline(void* ctx, std::int16_t value) {
        static_cast<SensorHandle*>(ctx)->last_value_ = value;
    }

    std::int16_t last_value_ = 0;
};

// ---- 3) Name mangling: vi sao can extern "C" ----
// Ham C++ thuong (co the nap chong) duoc doi ten luc bien dich de encode kieu tham so:
void describe(int) { std::puts("describe(int)"); }
void describe(double) { std::puts("describe(double)"); }
// KHONG THE lam dieu tuong tu voi extern "C": moi ten chi duoc dinh nghia MOT LAN, khong nap chong duoc,
// vi ten khong con encode kieu tham so - do la ly do file .h cua thu vien C khong ho tro overload.

int main() {
    describe(1);
    describe(2.5);

    SensorHandle sensor(1000, 4);
    sensor.poll();
    std::printf("gia tri cuoi nhan tu callback kieu C: %d\n", static_cast<int>(sensor.last_value()));
}
