// Phân tích gói tin từ UART có kiểm tra biên: [0xAA][len][payload...][checksum]
#include <cstddef>
#include <cstdint>
#include <cstdio>

struct Packet {
    std::uint8_t len;
    const std::uint8_t* payload;   // trỏ vào bộ đệm gốc, không sao chép
};

enum class ParseResult { Ok, TooShort, BadStart, BadLength, BadChecksum };

constexpr const char* to_string(ParseResult r) {
    switch (r) {
        case ParseResult::Ok:          return "Ok";
        case ParseResult::TooShort:    return "TooShort";
        case ParseResult::BadStart:    return "BadStart";
        case ParseResult::BadLength:   return "BadLength";
        case ParseResult::BadChecksum: return "BadChecksum";
    }
    return "?";
}

ParseResult parse(const std::uint8_t* buf, std::size_t size, Packet& out) {
    if (buf == nullptr || size < 3) return ParseResult::TooShort;   // start + len + checksum
    if (buf[0] != 0xAA) return ParseResult::BadStart;
    const std::size_t len = buf[1];
    if (size < 3 + len) return ParseResult::BadLength;               // kiểm tra biên TRƯỚC khi đọc payload

    std::uint8_t sum = 0;
    for (std::size_t i = 0; i < 2 + len; ++i) sum = static_cast<std::uint8_t>(sum + buf[i]);
    if (sum != buf[2 + len]) return ParseResult::BadChecksum;

    out.len = static_cast<std::uint8_t>(len);
    out.payload = buf + 2;
    return ParseResult::Ok;
}

int main() {
    const std::uint8_t good[]    = {0xAA, 0x03, 0x10, 0x20, 0x30, 0x0D};   // 0xAA+3+0x10+0x20+0x30 = 0x10D -> giữ byte thấp: 0x0D
    const std::uint8_t bad_sum[] = {0xAA, 0x03, 0x10, 0x20, 0x30, 0x00};
    const std::uint8_t cut[]     = {0xAA, 0x05, 0x01};
    const std::uint8_t noise[]   = {0x55, 0x01, 0x02, 0x03};

    struct Case { const char* name; const std::uint8_t* data; std::size_t size; };
    const Case cases[] = {
        {"hop le",       good,    sizeof good},
        {"sai checksum", bad_sum, sizeof bad_sum},
        {"bi cat",       cut,     sizeof cut},
        {"nhieu",        noise,   sizeof noise},
    };
    for (const Case& c : cases) {
        Packet p{};
        ParseResult r = parse(c.data, c.size, p);
        std::printf("%-13s -> %s", c.name, to_string(r));
        if (r == ParseResult::Ok)
            std::printf(" (len=%u, byte dau=0x%02X)", static_cast<unsigned>(p.len), static_cast<unsigned>(p.payload[0]));
        std::putchar('\n');
    }
    return 0;
}
