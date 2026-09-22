// MO PHONG TOI GIAN cac khai niem RTOS (task, hang doi, mutex) tren host, KHONG PHAI FreeRTOS/Zephyr
// that. Muc dich: thay duoc CACH cac khai niem lam viec cung nhau, truoc khi doc API that cua RTOS ban
// chon (ten ham/kieu du lieu chinh xac phai tra trong tai lieu FreeRTOS/Zephyr cho phien ban ban dung).
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>

// Hang doi co dinh, khong heap - giong tinh than xQueueCreate() cua FreeRTOS nhung don gian hoa toi da.
template <typename T, std::size_t N>
class FixedQueue {
public:
    bool send(const T& v) {
        if (count_ == N) return false;  // FreeRTOS that: xQueueSend co the CHO (block) toi khi co cho
        buf_[(head_ + count_) % N] = v;
        ++count_;
        return true;
    }
    bool receive(T& out) {
        if (count_ == 0) return false;  // FreeRTOS that: xQueueReceive co the CHO toi khi co du lieu
        out = buf_[head_];
        head_ = (head_ + 1) % N;
        --count_;
        return true;
    }

private:
    std::array<T, N> buf_{};
    std::size_t head_ = 0, count_ = 0;
};

// Mutex don gian hoa: tren mo phong MOT LUONG nay no khong the that su xay ra tranh chap, nhung API
// va mau RAII (Lock) giong het cach dung xSemaphoreCreateMutex()/xSemaphoreTake()/xSemaphoreGive() that.
class SimpleMutex {
public:
    bool try_lock() { if (locked_) return false; locked_ = true; return true; }
    void unlock() { locked_ = false; }

private:
    bool locked_ = false;
};

class Lock {
public:
    explicit Lock(SimpleMutex& m) : m_(m), owned_(m.try_lock()) {}
    ~Lock() { if (owned_) m_.unlock(); }
    explicit operator bool() const { return owned_; }

private:
    SimpleMutex& m_;
    bool owned_;
};

static FixedQueue<std::uint16_t, 4> g_sensor_queue;
static SimpleMutex g_log_mutex;
static std::uint32_t g_shared_log_count = 0;

void log_line(const char* msg) {
    Lock lock(g_log_mutex);
    if (!lock) { std::puts("  (bo qua log: dang co task khac ghi log)"); return; }
    std::printf("  log #%u: %s\n", static_cast<unsigned>(++g_shared_log_count), msg);
}

// "Task" san xuat: doc cam bien gia lap moi 2 tick, gui vao hang doi.
void producer_task(std::uint32_t tick) {
    if (tick % 2 != 0) return;
    const auto sample = static_cast<std::uint16_t>(1000 + tick);
    if (g_sensor_queue.send(sample)) log_line("da gui mau vao hang doi");
    else log_line("hang doi day, bo mau (trong FreeRTOS that: task co the CHO thay vi mat du lieu)");
}

// "Task" tieu thu: rut mau ra khoi hang doi moi tick, neu co.
void consumer_task(std::uint32_t /*tick*/) {
    std::uint16_t sample;
    if (g_sensor_queue.receive(sample)) std::printf("  consumer: xu ly mau %u\n", static_cast<unsigned>(sample));
}

int main() {
    // Dispatcher round-robin don gian: goi lan luot tung task moi "tick", KHONG uu tien, KHONG ngat
    // thuc su - FreeRTOS/Zephyr that dung bo lap lich uu tien va ngat phan cung (SysTick) de chuyen task.
    for (std::uint32_t tick = 0; tick < 6; ++tick) {
        std::printf("tick %u:\n", static_cast<unsigned>(tick));
        producer_task(tick);
        consumer_task(tick);
    }
}
