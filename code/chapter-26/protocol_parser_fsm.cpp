// FSM phan tich khung giao thuc dang: [STX][LEN][LEN byte du lieu][CHECKSUM][ETX], xu ly TUNG BYTE
// khi no den (tu ring buffer UART - chuong 24), khong can doi ca khung ve mot lan trong bo nho lon.
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>

constexpr std::uint8_t kStx = 0x02;
constexpr std::uint8_t kEtx = 0x03;

enum class ParserState : std::uint8_t { WaitStx, WaitLen, ReadData, WaitChecksum, WaitEtx };

class FrameParser {
public:
    // Tra ve true khi VUA hoan tat mot khung hop le (payload_()/length() doc duoc ngay sau do).
    bool feed(std::uint8_t byte) {
        switch (state_) {
            case ParserState::WaitStx:
                if (byte == kStx) { state_ = ParserState::WaitLen; }
                return false;
            case ParserState::WaitLen:
                if (byte > payload_.size()) { reset("do dai vuot qua bo dem"); return false; }
                len_ = byte;
                idx_ = 0;
                checksum_ = 0;
                state_ = (len_ == 0) ? ParserState::WaitChecksum : ParserState::ReadData;
                return false;
            case ParserState::ReadData:
                payload_[idx_++] = byte;
                checksum_ ^= byte;  // don gian hoa: XOR toan bo byte du lieu (khong phai chuan CRC that)
                if (idx_ == len_) state_ = ParserState::WaitChecksum;
                return false;
            case ParserState::WaitChecksum:
                if (byte != checksum_) { reset("checksum sai"); return false; }
                state_ = ParserState::WaitEtx;
                return false;
            case ParserState::WaitEtx:
                if (byte != kEtx) { reset("thieu ETX"); return false; }
                state_ = ParserState::WaitStx;
                return true;  // khung hoan tat va hop le
        }
        return false;
    }

    std::size_t length() const { return len_; }
    const std::uint8_t* payload() const { return payload_.data(); }

private:
    void reset(const char* reason) {
        std::printf("parser: huy khung (%s), quay ve cho STX\n", reason);
        state_ = ParserState::WaitStx;
    }

    ParserState state_ = ParserState::WaitStx;
    std::array<std::uint8_t, 16> payload_{};
    std::uint8_t len_ = 0;
    std::uint8_t idx_ = 0;
    std::uint8_t checksum_ = 0;
};

int main() {
    FrameParser parser;

    // Khung hop le: STX, LEN=3, 'A','B','C', checksum='A'^'B'^'C', ETX.
    const std::uint8_t good_frame[] = {kStx, 3, 'A', 'B', 'C', static_cast<std::uint8_t>('A' ^ 'B' ^ 'C'), kEtx};
    // Khung loi: checksum sai.
    const std::uint8_t bad_frame[] = {kStx, 2, 'X', 'Y', 0x00, kEtx};
    // Rac truoc mot khung hop le (parser phai bo qua cho den khi thay STX).
    const std::uint8_t noisy_then_good[] = {0xFF, 0xEE, kStx, 1, 'Z', 'Z', kEtx};

    auto run = [&](const std::uint8_t* data, std::size_t n) {
        for (std::size_t i = 0; i < n; ++i) {
            if (parser.feed(data[i])) {
                std::printf("KHUNG HOAN TAT, do dai=%zu, du lieu=\"", parser.length());
                for (std::size_t k = 0; k < parser.length(); ++k) std::putchar(static_cast<char>(parser.payload()[k]));
                std::printf("\"\n");
            }
        }
    };

    run(good_frame, sizeof good_frame);
    run(bad_frame, sizeof bad_frame);
    run(noisy_then_good, sizeof noisy_then_good);
}
