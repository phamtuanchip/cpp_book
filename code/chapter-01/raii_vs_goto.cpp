// Nhả tài nguyên khi có nhiều đường thoát: goto cleanup (C) và RAII (C++).
#include <cstdio>

static int g_locked = 0;
static void lock()   { ++g_locked; std::puts("  lock"); }
static void unlock() { --g_locked; std::puts("  unlock"); }

// ---- Cách C: mọi đường thoát phải nhảy về nhãn dọn dẹp ----
static int process_c(int fail_at) {
    int rc = -1;
    lock();
    if (fail_at == 1) goto out;
    if (fail_at == 2) goto out;
    rc = 0;
out:
    unlock();
    return rc;
}

// ---- Cách C++: RAII, destructor chạy ở MỌI đường thoát ----
class LockGuard {
public:
    LockGuard()  { lock(); }
    ~LockGuard() { unlock(); }
    LockGuard(const LockGuard&) = delete;
    LockGuard& operator=(const LockGuard&) = delete;
};

static int process_cpp(int fail_at) {
    LockGuard guard;
    if (fail_at == 1) return -1;
    if (fail_at == 2) return -2;
    return 0;
}

int main() {
    for (int fail_at = 0; fail_at <= 2; ++fail_at) {
        std::printf("C   fail_at=%d\n", fail_at);
        int rc = process_c(fail_at);
        std::printf("  rc=%d locked=%d\n", rc, g_locked);

        std::printf("C++ fail_at=%d\n", fail_at);
        rc = process_cpp(fail_at);
        std::printf("  rc=%d locked=%d\n", rc, g_locked);
    }
    return 0;
}
