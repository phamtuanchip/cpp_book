// Bản ĐÚNG: destructor của lớp cơ sở khai báo 'virtual', nên xoá qua con trỏ lớp cơ sở
// gọi đúng chuỗi destructor con -> cơ sở, không rò rỉ tài nguyên.
#include <cstdio>

int g_derived_destructed = 0;

struct BaseVirtual {
    virtual void ping() { std::puts("BaseVirtual::ping"); }
    virtual ~BaseVirtual() { std::puts("~BaseVirtual"); }   // 'virtual': bắt buộc vì lớp có hàm ảo
};

struct DerivedB : BaseVirtual {
    ~DerivedB() override { ++g_derived_destructed; std::puts("~DerivedB (giai phong tai nguyen o day)"); }
};

int main() {
    BaseVirtual* p = new DerivedB();
    delete p;   // gọi ~DerivedB() trước, rồi mới ~BaseVirtual() -- đúng thứ tự, không cảnh báo
    std::printf("g_derived_destructed = %d (dung: 1)\n", g_derived_destructed);
    return 0;
}
