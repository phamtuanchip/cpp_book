// Placement new: xay dung mot doi tuong tai mot vung nho DA CO SAN (khong cap phat gi them).
// Huu ich khi bo nho phai la static/stack (khong heap) nhung muon tri hoan luc goi constructor,
// hoac tai su dung lai cung mot vung cho cac doi tuong khac nhau theo thoi gian.
#include <cstdint>
#include <cstdio>
#include <new>

class Logger {
public:
    explicit Logger(const char* tag) : tag_(tag) { std::printf("Logger('%s') khoi tao\n", tag_); }
    ~Logger() { std::printf("Logger('%s') huy\n", tag_); }
    void log(const char* msg) const { std::printf("[%s] %s\n", tag_, msg); }

private:
    const char* tag_;
};

int main() {
    // Vung nho tinh, can le dung cho Logger, CHUA co doi tuong nao ton tai trong do.
    alignas(Logger) unsigned char storage[sizeof(Logger)];

    // Placement new: goi constructor tai dia chi 'storage', khong cap phat bo nho moi.
    Logger* log1 = new (storage) Logger("boot");
    log1->log("he thong khoi dong");

    // Goi destructor THU CONG (bat buoc voi placement new - khong dung 'delete').
    log1->~Logger();

    // Tai su dung CUNG mot vung nho cho mot doi tuong khac, vong doi khac.
    Logger* log2 = new (storage) Logger("sensor");
    log2->log("doc cam bien xong");
    log2->~Logger();

    std::printf("sizeof(storage) = %zu, khong co byte nao tu heap duoc dung\n", sizeof(storage));
}
