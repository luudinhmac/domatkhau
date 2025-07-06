# Sử dụng Alpine Linux 3.20 làm base image để build (nhỏ gọn, nhanh)
FROM alpine:3.20 AS builder

# Cài gcc và thư viện musl-dev để biên dịch chương trình C
RUN apk add --no-cache gcc musl-dev

# Đặt thư mục làm việc trong container là /src
WORKDIR /src

# Copy code check_password.c từ máy host vào /src trong container
COPY check_password.c .

# Biên dịch chương trình với gcc, tạo file thực thi tĩnh static linking
# -static: link tĩnh để không phụ thuộc thư viện ngoài khi chạy
# -lcrypt: link thư viện crypt để hỗ trợ hàm crypt()
RUN gcc -static -o check_password check_password.c -lcrypt

# Bắt đầu giai đoạn mới với base image trống (scratch) để tạo image nhỏ gọn
FROM scratch

# Copy file thực thi từ stage builder
COPY --from=builder /src/check_password /check_password

# Thiết lập chương trình mặc định khi container chạy là /check_password
ENTRYPOINT ["/check_password"]

