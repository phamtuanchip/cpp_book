// Minh hoa lop loi ma "borrow checker" cua Rust bat duoc LUC BIEN DICH, con C++ thi khong (chuong 33).
// Phan "TRUOC" chi la CHU THICH minh hoa loi kinh dien - KHONG duoc bien dich trong file nay (co hanh
// vi khong xac dinh, khong phu hop dua vao mot chuong trinh "verified"). Phan bien dich la CACH AN TOAN.
#include <cstdio>
#include <vector>

// TRUOC (chi minh hoa, khong bien dich):
//   std::vector<int> v{1, 2, 3};
//   int* p = &v[0];          // p tro vao vung nho noi bo cua vector
//   for (int i = 0; i < 100; ++i) v.push_back(i);  // co the lam vector CAP PHAT LAI vung nho moi
//   std::printf("%d\n", *p); // HANH VI KHONG XAC DINH: p co the da tro toi vung nho da giai phong.
//   Loi nay KHONG duoc trinh bien dich C++ canh bao (voi -Wall -Wextra) - no chi lo ra luc chay,
//   va co the "tinh co chay dung" nhieu lan truoc khi that bai that su. Trong Rust, borrow checker
//   TU CHOI BIEN DICH code tuong duong: giu 'p' (mot "borrow" bat bien) trong luc goi push_back
//   (mot thao tac can muon "borrow" kha bien) la loi bien dich, khong phai loi runtime.

// SAU: dung CHI SO thay cho con tro/tham chieu khi vector co the thay doi kich thuoc giua chung.
int safe_pattern_using_index() {
    std::vector<int> v{1, 2, 3};
    const std::size_t idx = 0;  // nho VI TRI, khong nho DIA CHI
    for (int i = 0; i < 100; ++i) v.push_back(i);
    return v[idx];  // luon lay lai gia tri qua chi so, sau khi vector da on dinh kich thuoc
}

// SAU (cach khac): lay tham chieu/con tro CHI SAU KHI da chac chan khong con thao tac lam vector doi cho.
int safe_pattern_reference_after_mutation() {
    std::vector<int> v{1, 2, 3};
    for (int i = 0; i < 100; ++i) v.push_back(i);  // moi thay doi kich thuoc xong o day
    const int& first = v[0];                        // LAY THAM CHIEU SAU CUNG, khong con push_back nao nua
    return first;
}

int main() {
    std::printf("safe_pattern_using_index() = %d\n", safe_pattern_using_index());
    std::printf("safe_pattern_reference_after_mutation() = %d\n", safe_pattern_reference_after_mutation());
}
