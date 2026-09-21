// Move-only handle sở hữu một ô trong pool tĩnh: RAII + move semantics, không heap.
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <utility>

class BufferPool {
public:
    static constexpr std::size_t kSlots = 4;
    static constexpr std::size_t kSize = 64;

    class Handle {
    public:
        Handle() = default;
        Handle(const Handle&) = delete;
        Handle& operator=(const Handle&) = delete;
        Handle(Handle&& o) noexcept : pool_(o.pool_), idx_(o.idx_) { o.pool_ = nullptr; }
        Handle& operator=(Handle&& o) noexcept {
            if (this != &o) {
                release();
                pool_ = o.pool_;
                idx_ = o.idx_;
                o.pool_ = nullptr;
            }
            return *this;
        }
        ~Handle() { release(); }

        explicit operator bool() const { return pool_ != nullptr; }
        std::uint8_t* data() { return pool_->storage_[idx_].data(); }

    private:
        friend class BufferPool;
        Handle(BufferPool* p, std::size_t i) : pool_(p), idx_(i) {}
        void release() {
            if (pool_) {
                pool_->used_[idx_] = false;
                pool_ = nullptr;
            }
        }
        BufferPool* pool_ = nullptr;
        std::size_t idx_ = 0;
    };

    Handle acquire() {
        for (std::size_t i = 0; i < kSlots; ++i) {
            if (!used_[i]) {
                used_[i] = true;
                return Handle{this, i};
            }
        }
        return Handle{};  // hết ô: handle rỗng, không ném exception
    }

    std::size_t free_count() const {
        std::size_t n = 0;
        for (bool u : used_) n += u ? 0 : 1;
        return n;
    }

private:
    std::array<std::array<std::uint8_t, kSize>, kSlots> storage_{};
    std::array<bool, kSlots> used_{};
};

// Nhận handle theo giá trị: quyền sở hữu chuyển vào đây, ô được trả khi hàm kết thúc.
void consume(BufferPool::Handle h) {
    h.data()[0] = 0xAB;
    std::puts("consume: dang xu ly buffer");
}

int main() {
    BufferPool pool;
    std::printf("trong: %zu\n", pool.free_count());

    auto a = pool.acquire();
    auto b = pool.acquire();
    std::printf("sau 2 acquire: %zu\n", pool.free_count());

    consume(std::move(b));  // b bị "rút ruột": không dùng lại được
    std::printf("sau consume: %zu (b con hop le? %s)\n", pool.free_count(), b ? "co" : "khong");

    {
        auto c = pool.acquire();
        std::printf("trong scope: %zu\n", pool.free_count());
    }
    std::printf("ra scope: %zu\n", pool.free_count());
    (void)a;
}
