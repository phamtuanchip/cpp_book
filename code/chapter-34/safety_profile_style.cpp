// Minh hoa tinh than "safety profile" (Bounds safety, Type safety) ma C++ Core Guidelines va cac de
// xuat chuan hoa gan day huong toi: kiem tra bien luc chay bang assert/fail-fast, va mot wrapper
// "khong bao gio null" - tu viet don gian, GSL (Guidelines Support Library) that co san ban day du hon.
#include <cassert>
#include <cstdio>
#include <span>

// Bounds safety: boc mot span, MOI lan truy cap deu kiem tra chi so - khac operator[] tho cua span
// (khong kiem tra) hoac con tro C (khong biet do dai). Vi pham goi assert() (dung/tat qua NDEBUG).
template <typename T>
class CheckedSpan {
public:
    explicit CheckedSpan(std::span<T> s) : s_(s) {}

    T& at(std::size_t i) {
        assert(i < s_.size() && "CheckedSpan::at: chi so vuot gioi han");
        return s_[i];
    }
    std::size_t size() const { return s_.size(); }

private:
    std::span<T> s_;
};

// Type safety / null safety: mot con tro "khong bao gio null", kiem tra MOT LAN luc tao, sau do
// moi noi nhan NotNull<T*> KHONG can kiem tra lai null nua - loai bo ca lop if(ptr) rai rac.
template <typename T>
class NotNull {
public:
    explicit NotNull(T* p) : p_(p) { assert(p_ != nullptr && "NotNull: con tro null"); }
    T* get() const { return p_; }
    T& operator*() const { return *p_; }
    T* operator->() const { return p_; }

private:
    T* p_;
};

void print_value(NotNull<int> p) {
    // Khong can 'if (p.get())' o day: kieu NotNull da la LOI HUA (contract) tu noi goi.
    std::printf("gia tri = %d\n", *p);
}

int main() {
    int data[] = {10, 20, 30};
    CheckedSpan<int> cs{std::span<int>(data)};

    std::printf("cs.at(1) = %d\n", cs.at(1));
    for (std::size_t i = 0; i < cs.size(); ++i) std::printf("cs.at(%zu) = %d\n", i, cs.at(i));

    int value = 99;
    print_value(NotNull<int>{&value});

    std::puts("Thu cs.at(10) (vuot gioi han) se lam chuong trinh dung ngay tai day (assert),");
    std::puts("thay vi doc/ghi vao vung nho khong thuoc ve minh mot cach am tham.");
}
