#define _XOPEN_SOURCE
// Cho phép sử dụng các hàm mở rộng như crypt() trong <unistd.h>

#include <stdio.h>      // Thư viện nhập xuất chuẩn (printf, fprintf, fopen, fgets, ...)
#include <string.h>     // Thư viện xử lý chuỗi (strlen, strncmp, strtok_r, strcmp, ...)
#include <stdlib.h>     // Thư viện chuẩn (return, malloc, exit, ...)
#include <unistd.h>     // Cung cấp crypt() để mã hóa mật khẩu

int main(int argc, char *argv[]) {
    // Kiểm tra số lượng tham số dòng lệnh, chương trình yêu cầu 2 tham số
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <username> <listfile>\n", argv[0]);
        return 1; // Thoát với mã lỗi nếu không đủ tham số
    }

    const char *username = argv[1];  // Tên người dùng cần kiểm tra
    const char *listfile = argv[2];  // File chứa danh sách mật khẩu cần thử

    char filepath[512];              // Bộ đệm để chứa đường dẫn tuyệt đối (nếu cần)
    if (listfile[0] != '/') {
        // Nếu file không bắt đầu bằng '/', thêm '/' vào đầu để tạo đường dẫn tuyệt đối
        snprintf(filepath, sizeof(filepath), "/%s", listfile);
        listfile = filepath;
    }

    FILE *fp = fopen("/etc/shadow", "r");
    // Mở file /etc/shadow để đọc hash mật khẩu của user
    if (!fp) {
        perror("Cannot open /etc/shadow"); // In lỗi nếu không mở được (cần quyền root)
        return 1;
    }

    char line[1024];     // Bộ đệm đọc từng dòng trong /etc/shadow
    char *hash = NULL;   // Con trỏ chứa hash lấy ra từ file shadow
    int user_found = 0;  // Cờ đánh dấu đã tìm thấy user hay chưa

    // Duyệt từng dòng trong file /etc/shadow để tìm user
    while (fgets(line, sizeof(line), fp)) {
        // Nếu dòng bắt đầu bằng username và theo sau là dấu ':'
        if (strncmp(line, username, strlen(username)) == 0 && line[strlen(username)] == ':') {
            user_found = 1; // Đã tìm thấy user
            char *saveptr;
            strtok_r(line, ":", &saveptr);       // Bỏ qua phần username
            hash = strtok_r(NULL, ":", &saveptr); // Lấy phần hash mật khẩu
            break; // Thoát vòng lặp khi đã tìm thấy
        }
    }

    fclose(fp); // Đóng file sau khi đọc xong

    if (!user_found) {
        // Nếu không tìm thấy user trong /etc/shadow, thông báo và thoát
        printf("Không có người dùng %s trong hệ thống\n", username);
        return 1;
    }

    // In thông báo đã tìm thấy user và bắt đầu thử mật khẩu
    printf("Có người dùng %s trong hệ thống\n", username);

    // Kiểm tra hash có hợp lệ: phải bắt đầu bằng dấu '$'
    // (tất cả định dạng hash hiện đại như $1$, $5$, $6$ đều vậy)
    if (!hash || hash[0] != '$') {
        fprintf(stderr, "Hash không hợp lệ hoặc không nhận diện được thuật toán\n");
        return 1;
    }

    // Trích salt từ hash: lấy phần đến dấu '$' thứ 3, ví dụ: $6$salt$
    char salt[256];             // Bộ đệm lưu salt
    const char *p = hash;       // Con trỏ duyệt hash
    int dollar_count = 0;       // Đếm số dấu '$'
    int i = 0;
    while (*p && dollar_count < 3 && i < sizeof(salt) - 1) {
        if (*p == '$') dollar_count++; // Đếm dấu $
        salt[i++] = *p++;              // Lưu từng ký tự vào salt
    }
    salt[i] = '\0'; // Kết thúc chuỗi salt

    /*
    // (Tùy chọn) In thuật toán và salt được phát hiện từ hash
    printf("Thuật toán: %s, Salt: %s\n",
        (strncmp(hash, "$1$", 3) == 0) ? "MD5" :
        (strncmp(hash, "$5$", 3) == 0) ? "SHA-256" :
        (strncmp(hash, "$6$", 3) == 0) ? "SHA-512" : "Không rõ",
        salt);
    */

    // Mở file chứa danh sách mật khẩu cần thử
    FILE *list = fopen(listfile, "r");
    if (!list) {
        fprintf(stderr, "Không đọc được %s\n", listfile);
        return 2;
    }

    printf("Bắt đầu dò mật khẩu của %s đọc trong %s\n", username, listfile);
    
    char password[256]; // Bộ đệm chứa từng mật khẩu thử
    int found = 0;      // Cờ đánh dấu đã tìm thấy mật khẩu đúng chưa

    // Đọc từng dòng trong file mật khẩu
    while (fgets(password, sizeof(password), list)) {
        password[strcspn(password, "\n")] = '\0';  // Xóa ký tự xuống dòng '\n'

        // Mã hóa mật khẩu thử với salt đã trích xuất
        char *crypted = crypt(password, salt);

        // So sánh kết quả mã hóa với hash trong /etc/shadow
        if (crypted && strcmp(crypted, hash) == 0) {
            printf("%s là mật khẩu\n", password); // Mật khẩu khớp
            found = 1;
        } else {
            printf("%s không phải mật khẩu\n", password); // Mật khẩu sai
        }
    }

    fclose(list); // Đóng file danh sách mật khẩu

    return found ? 0 : 1; // Trả về 0 nếu tìm thấy, 1 nếu không
}

