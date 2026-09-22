// FunctionRef<Sig>: tham chieu KHONG SO HUU toi mot callable co operator() (lambda co capture, functor) -
// khong heap, khong gioi han kich thuoc capture (khac std::function), nhung: doi tuong duoc tham chieu
// phai CON SONG khi FunctionRef duoc goi (giong tham chieu thuong - xem canh bao chuong 16). Ban toi
// gian nay CHUA ho tro nhan thang mot con tro ham (can xu ly rieng vi con tro ham khong chuyen doi
// ngam dinh sang void* - xem phan van ban trong sach).
#include <cstdio>

template <typename Signature>
class FunctionRef;

template <typename R, typename... Args>
class FunctionRef<R(Args...)> {
public:
    template <typename F>
    FunctionRef(F& f) : obj_(&f), invoke_(&invoke_impl<F>) {}  // nhan tham chieu, khong sao chep F

    R operator()(Args... args) const { return invoke_(obj_, args...); }

private:
    template <typename F>
    static R invoke_impl(void* obj, Args... args) {
        return (*static_cast<F*>(obj))(args...);
    }

    void* obj_;
    R (*invoke_)(void*, Args...);
};

// Ham nhan FunctionRef: khong quan tam ben trong la lambda hay functor nao, khong heap, mot lan goi
// gian tiep qua con tro ham (khong co bang ao, khac voi lop co ham ao - chuong 14).
int apply_twice(FunctionRef<int(int)> f, int x) { return f(f(x)); }

int main() {
    int factor = 3;
    auto multiply = [factor](int x) { return x * factor; };  // lambda CO capture
    std::printf("apply_twice(multiply, 5) = %d\n", apply_twice(multiply, 5));  // (5*3)*3 = 45

    struct AddOne { int operator()(int x) const { return x + 1; } } add_one;
    std::printf("apply_twice(add_one, 5) = %d\n", apply_twice(add_one, 5));  // (5+1)+1 = 7

    std::printf("sizeof(FunctionRef<int(int)>) = %zu (chi 2 con tro, bat ke callable ben trong lon co nao)\n",
                sizeof(FunctionRef<int(int)>));
}
