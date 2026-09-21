// unique_ptr với custom deleter: quản lý tài nguyên KHÔNG nằm trên heap (ở đây: quyền dùng một bus SPI).
#include <cstdio>
#include <memory>

struct SpiBus {
    int id;
    bool busy;
};

static SpiBus g_spi[2] = {{1, false}, {2, false}};

struct SpiDeleter {
    void operator()(SpiBus* bus) const {
        std::printf("tra SPI%d\n", bus->id);
        bus->busy = false;
    }
};

using SpiLease = std::unique_ptr<SpiBus, SpiDeleter>;

SpiLease lease_spi(int index) {
    SpiBus& bus = g_spi[index];
    if (bus.busy) return SpiLease{};
    bus.busy = true;
    return SpiLease{&bus};
}

int main() {
    {
        auto lease = lease_spi(0);
        std::printf("muon SPI%d\n", lease->id);
        auto again = lease_spi(0);
        std::printf("muon lan 2: %s\n", again ? "duoc" : "tu choi");
    }  // lease ra khỏi scope -> deleter chạy
    auto after = lease_spi(0);
    std::printf("sau khi tra: %s\n", after ? "duoc" : "tu choi");

    std::printf("sizeof(SpiLease)=%zu sizeof(void*)=%zu\n", sizeof(SpiLease), sizeof(void*));
}
