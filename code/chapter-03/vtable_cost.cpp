// Đo chi phí bộ nhớ của đa hình động: một con trỏ vptr cho mỗi đối tượng.
#include <cstdio>

struct Plain {
    int x;
    int get() const { return x; }
};

struct Virtual {
    int x;
    virtual int get() const { return x; }
    virtual ~Virtual() = default;
};

int main() {
    std::printf("sizeof(Plain)   = %zu\n", sizeof(Plain));
    std::printf("sizeof(Virtual) = %zu\n", sizeof(Virtual));
    std::printf("sizeof(void*)   = %zu\n", sizeof(void*));
    return 0;
}
