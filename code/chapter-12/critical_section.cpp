// RAII đếm lồng nhau: vùng tới hạn (critical section) được bật/tắt đúng một lần
// dù các đối tượng ScopedIrqLock lồng nhau bao nhiêu tầng.
#include <cstdint>
#include <cstdio>

// Mô phỏng "phần cứng": một cờ toàn cục cho biết ngắt đang bật hay tắt.
static bool g_irq_enabled = true;
static void irq_disable() { g_irq_enabled = false; }
static void irq_enable()  { g_irq_enabled = true; }

class ScopedIrqLock {
public:
    ScopedIrqLock() {
        if (depth_ == 0) irq_disable();   // chỉ tắt ngắt ở lần lồng đầu tiên
        ++depth_;
    }
    ~ScopedIrqLock() {
        --depth_;
        if (depth_ == 0) irq_enable();    // chỉ bật lại khi ra khỏi tầng ngoài cùng
    }
    ScopedIrqLock(const ScopedIrqLock&) = delete;
    ScopedIrqLock& operator=(const ScopedIrqLock&) = delete;

private:
    static inline std::uint8_t depth_ = 0;   // biến static dùng chung cho mọi đối tượng của lớp
};

static volatile std::uint32_t g_shared_counter = 0;

static void increment_shared(int by) {
    ScopedIrqLock lock;                 // vào vùng tới hạn
    g_shared_counter += static_cast<std::uint32_t>(by);
    std::printf("  trong increment_shared: irq_enabled=%d counter=%u\n", g_irq_enabled,
                static_cast<unsigned>(g_shared_counter));
}

static void update_twice() {
    ScopedIrqLock outer;                // tầng ngoài
    std::printf("vao update_twice: irq_enabled=%d\n", g_irq_enabled);
    increment_shared(1);                // tầng trong: KHÔNG được bật lại ngắt ở đây
    increment_shared(2);
    std::printf("truoc khi roi update_twice: irq_enabled=%d\n", g_irq_enabled);
}

int main() {
    std::printf("truoc: irq_enabled=%d\n", g_irq_enabled);
    update_twice();
    std::printf("sau  : irq_enabled=%d counter=%u\n", g_irq_enabled, static_cast<unsigned>(g_shared_counter));
    return 0;
}
