#include <cstdio>

int main() {
    std::printf("__cplusplus = %ldL\n", static_cast<long>(__cplusplus));
#if defined(__clang__)
    std::printf("compiler    = clang %d.%d\n", __clang_major__, __clang_minor__);
#elif defined(__GNUC__)
    std::printf("compiler    = gcc %d.%d\n", __GNUC__, __GNUC_MINOR__);
#elif defined(_MSC_VER)
    std::printf("compiler    = msvc %d\n", _MSC_VER);
#endif
    return 0;
}
