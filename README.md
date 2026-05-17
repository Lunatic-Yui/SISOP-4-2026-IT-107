# SISOP-4-2026-IT-107

## Member

| Nama                   | NRP        |
| ---------------------- | ---------- |
| Yovi Prayudya Rizky Ramadhani | 5027251107 |

## Reporting

### Soal 1

#### Penjelasan

**amba_files.zip**

Diberikan link zip: [amba_files.zip](https://drive.google.com/file/d/1nLXFhptDo2mnUlZsw8pTWyAVpV49W20U/view?usp=drive_link) dan bisa langsung saja pakai `wget` command. Tapi saya disini menggunakan `gdown` untuk langsung mengambil filenya. Commandnya: `gdown https://drive.google.com/file/d/1nLXFhptDo2mnUlZsw8pTWyAVpV49W20U/view?usp=drive_link` dan terambil filenya. Ketika file zip sudah didapat, kita harus mengunzip file itu dan menghapus file zip jika sudah di unzip. File untuk menghapus zip itu sendiri: `rm amba_files.zip`. 

**Fuse.c**

Kodenya seperti ini:

```c
#define FUSE_USE_VERSION 28
#define _XOPEN_SOURCE 700
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>

static char source_dir[1024];

static void get_full_path(char *fpath, const char *path) {
    strcpy(fpath, source_dir);
    strcat(fpath, path);
}
```

Ini adalah setupnya untuk pertama dengan mengimport library yang dibutuhkan dan mendefinisikan untuk full path directory yang dibutuhkan. Selanjutnya:

```c
static void build_tujuan_content(char *output) {
    strcpy(output, "Tujuan Mas Amba: ");
    char frag_buf[1024] = "";

    for (int i = 1; i <= 7; i++) {
        char filepath[1024];
        sprintf(filepath, "%s/%d.txt", source_dir, i);
        FILE *f = fopen(filepath, "r");
        if (!f) continue;

        char line[1024];
        while (fgets(line, sizeof(line), f)) {
            char *pos = strstr(line, "KOORD: ");
            if (pos) {
                pos += 7; 
                char *newline = strpbrk(pos, "\r\n");
                if (newline) *newline = '\0';
                strcat(frag_buf, pos);
                break;
            }
        }
        fclose(f);
    }
    strcat(output, frag_buf);
    strcat(output, "\n");
}
```

adalah program yang membuat kontent dari tujuan mas amba dengan mencari file txt dari 1 sampai 7. Nah dari kode ini juga mencari koordinat dari file txt yang sudah dibuat. Ketika koordinat ini ketemu, dia akan menyimpannya dan mencari sampai akhir dan ketika sudah semua akan digabung ke dalam 1 kalimat. Selanjutnya

```c
static int xmp_getattr(const char *path, struct stat *stbuf) {
    if (strcmp(path, "/tujuan.txt") == 0) {
        char content[2048];
        build_tujuan_content(content);
        
        memset(stbuf, 0, sizeof(struct stat));
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = strlen(content); 
        return 0;
    }

    char fpath[1024];
    get_full_path(fpath, path);
    int res = lstat(fpath, stbuf);
    if (res == -1) return -errno;
    return 0;
}
```

adalah kode yang bertindak sebagai penyedia metadata (atribut) file. Pada kode ini, diterapkan logika pembuatan virtual file. Ketika path yang direquest adalah `/tujuan.txt`, fungsi akan mengabaikan pencarian di penyimpanan fisik dan secara dinamis memalsukan metadata stat buffer-nya, menetapkannya sebagai file reguler read-only (`S_IFREG | 0444`) dengan ukuran file (size) yang disesuaikan dari output fungsi `build_tujuan_content`. Namun, jika path yang direquest bukan `/tujuan.txt`, fungsi akan mengarahkan request tersebut ke file fisik aslinya menggunakan fungsi `lstat` bawaan OS (Passthrough). Selanjutnya:

```c
static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    char fpath[1024];
    get_full_path(fpath, path);

    DIR *dp = opendir(fpath);
    if (dp == NULL) return -errno;

    struct dirent *de;
    while ((de = readdir(dp)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        filler(buf, de->d_name, &st, 0);
    }
    closedir(dp);

    if (strcmp(path, "/") == 0) {
        filler(buf, "tujuan.txt", NULL, 0);
    }

    return 0;
}
```

adalah kode yang berfungsi untuk menangani sistem pembacaan direktori (directory listing) yang dipicu oleh perintah seperti `ls`. Fungsi ini bekerja dalam dua tahap. Tahap pertama adalah passthrough, di mana program membaca isi dari direktori fisik menggunakan `opendir` dan `readdir`, lalu memasukkan setiap entri yang valid ke dalam buffer FUSE melalui fungsi filler. Tahap kedua adalah injeksi virtual file; program melakukan pengecekan jika path direktori saat ini adalah root directory (`/`). Jika memenuhi kondisi tersebut, program akan secara eksplisit memanggil fungsi `filler` untuk menyisipkan string `"tujuan.txt"`. Teknik ini memastikan file virtual `tujuan.txt` dapat terdaftar dan terlihat oleh user dalam sistem berkas FUSE meskipun file fisiknya tidak pernah ada di dalam direktori penyimpanan sebenarnya. Lalu

```c
static int xmp_open(const char *path, struct fuse_file_info *fi) {
    if (strcmp(path, "/tujuan.txt") == 0) {
        if ((fi->flags & O_ACCMODE) != O_RDONLY) return -EACCES;
        return 0;
    }

    char fpath[1024];
    get_full_path(fpath, path);
    int res = open(fpath, fi->flags);
    if (res == -1) return -errno;
    close(res);
    return 0;
}
```

adalah kode yang berperan sebagai mekanisme validasi hak akses saat sistem memanggil system call pembukaan file. Fungsi ini mengimplementasikan mekanisme perlindungan terhadap virtual file `/tujuan.txt`. Menggunakan bitwise AND mask pada `fi->flags` dengan `O_ACCMODE`, fungsi memastikan bahwa virtual file tersebut secara ketat hanya dapat dibuka dalam mode read-only (`O_RDONLY`). Modifikasi atau percobaan penulisan akan langsung ditolak dengan kode error `-EACCES`. Sementara itu, untuk path selain virtual file, fungsi akan melakukan passthrough dengan menguji eksekusi system call `open` pada file fisik aslinya. Jika file descriptor berhasil didapatkan, resource tersebut akan segera di-close, mengingat peran fungsi ini dalam konteks tersebut murni sebagai access permission validation sebelum proses delegasi data ke fungsi `read` atau `write`.

```c
static int xmp_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    if (strcmp(path, "/tujuan.txt") == 0) {
        char content[2048];
        build_tujuan_content(content);
        size_t len = strlen(content);
        
        if (offset >= len) return 0;
        if (offset + size > len) size = len - offset;
        
        memcpy(buf, content + offset, size);
        return size;
    }

    char fpath[1024];
    get_full_path(fpath, path);
    int fd = open(fpath, O_RDONLY);
    if (fd == -1) return -errno;

    int res = pread(fd, buf, size, offset);
    if (res == -1) res = -errno;
    close(fd);
    return res;
}
```

Nah pada kode ini, ia bertanggung jawab untuk melayani system call pembacaan data (read) ke dalam buffer. Untuk virtual file `/tujuan.txt`, fungsi ini memanggil build_tujuan_content untuk men-generate isi data secara on-the-fly di dalam memori. Selanjutnya, fungsi melakukan kalkulasi aritmatika pointer dan size buffer berdasarkan nilai `offset` yang diminta sistem untuk mencegah terjadinya `memory out-of-bounds` (memory yang membaca atau menuliskan program ke memory yang sudah dialokasikan ke tempat lain), sebelum akhirnya menggunakan `memcpy` untuk mentransfer data ke buffer FUSE. Sedangkan untuk file reguler lainnya, fungsi beroperasi dalam mode passthrough dengan membuka physical file descriptor dan menggunakan system call `pread`. Penggunaan `pread` krusial dalam lingkungan FUSE karena sifatnya yang thread-safe, memungkinkan pembacaan pada offset spesifik tanpa mengubah global file offset pointer. Dan terakhir

```c
static struct fuse_operations xmp_oper = {
    .getattr = xmp_getattr,
    .readdir = xmp_readdir,
    .open    = xmp_open,
    .read    = xmp_read,
};

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <source_dir> <mount_dir>\n", argv[0]);
        return 1;
    }

    realpath(argv[1], source_dir);

    char *fuse_argv[] = {argv[0], argv[2], NULL};
    int fuse_argc = 2;

    return fuse_main(fuse_argc, fuse_argv, &xmp_oper, NULL);
}
```

adalah lanjutan dari kode sebelumnya. Dalam kode ini terdiri dari dua bagian utama untuk inisiasi sistem FUSE. Pertama, `struct fuse_operations xmp_oper` bertindak sebagai struktur data yang berisi mapping (pemetaan) function pointer. Struktur ini mendaftarkan fungsi-fungsi kustom yang telah dibuat (seperti `xmp_getattr`, `xmp_readdir`, dll) agar dapat dikenali dan dieksekusi oleh library FUSE ketika ada system call dari OS. Kedua, fungsi `main` berfungsi sebagai entry point program. Fungsi ini bertugas melakukan validasi input argumen dari user (berupa direktori sumber dan direktori mount), mengkonversi path sumber menjadi absolute path menggunakan `realpath`, menyiapkan format argumen khusus untuk FUSE, dan terakhir mengeksekusi `fuse_main` untuk menjalankan daemon FUSE di background

```c
for (i = 0; i < MAX_CLIENTS; i++) {
            sd = clients[i].socket;
            if (FD_ISSET(sd, &readfds)) {
                memset(buffer, 0, 1024);
                valread = read(sd, buffer, 1024);
                
                if (valread == 0 || strcmp(buffer, EXIT_CMD) == 0) {
                    char log_msg[100];
                    if (clients[i].is_admin) {
                        snprintf(log_msg, sizeof(log_msg), "User 'The Knights' disconnected");
                    } else {
                        snprintf(log_msg, sizeof(log_msg), "User '%s' disconnected", clients[i].username);
                    }
                    if(strlen(clients[i].username) > 0) write_log("System", log_msg);
                    
                    close(sd);
                    clients[i].socket = 0;
                    clients[i].is_admin = 0;
                    memset(clients[i].username, 0, 50);
                } else {
                    buffer[strcspn(buffer, "\r\n")] = 0; 
                    
                    if (strncmp(buffer, "LOGIN:", 6) == 0) {
                        char *name = buffer + 6;
                        int dup = 0;
                        for(int j=0; j<MAX_CLIENTS; j++) {
                            if(clients[j].socket != 0 && strcmp(clients[j].username, name) == 0) dup=1;
                        }
                        
                        if (dup) send(sd, "ERR_DUP", 7, 0);
                        else {
                            strcpy(clients[i].username, name);
                            send(sd, "OK", 2, 0);
                            
                            char log_msg[100];
                            snprintf(log_msg, sizeof(log_msg), "User '%s' connected", name);
                            write_log("System", log_msg);
                        }
                    }
```

adalah kode yang mengatur socketnya. Kalau ada socket lain yang masuk, ia akan dicheck terlebih dahulu apakah namanya sama atau tidak (error handling). Jika sama, dia menolaknya. Jika tidak, maka dia akan menerimanya dan mendengarkan socket yang lain itu. Ketika ada command `/exit` maka dia akan terputus dan arraynya pun dikosongkan. Selanjutnya:

```c
else if (strncmp(buffer, "ADMIN:", 6) == 0) {

                        strcpy(clients[i].username, "The Knights");
                        clients[i].is_admin = 1;
                        send(sd, "OK_ADMIN", 8, 0);
                        write_log("System", "User 'The Knights' connected");
                    } else if (clients[i].is_admin) {
                        if (strcmp(buffer, "CMD:USERS") == 0) {
                            int count = 0;
                            for (int j = 0; j < MAX_CLIENTS; j++) {
                                if (clients[j].socket != 0 && clients[j].is_admin == 0 && strlen(clients[j].username) > 0) count++;
                            }
                            char resp[100];
                            snprintf(resp, sizeof(resp), "[Sistem] Total Entitas Aktif: %d", count);
                            send(sd, resp, strlen(resp), 0);
                            write_log("Admin", "RPC_GET_USERS");
                        } else if (strcmp(buffer, "CMD:UPTIME") == 0) {
                            double seconds = difftime(time(NULL), start_time);
                            char resp[100];
                            snprintf(resp, sizeof(resp), "[Sistem] Waktu Aktif Server: %.0f detik", seconds);
                            send(sd, resp, strlen(resp), 0);
                            write_log("Admin", "RPC_GET_UPTIME");
                        } else if (strcmp(buffer, "CMD:SHUTDOWN") == 0) {
                            write_log("Admin", "RPC_SHUTDOWN");
                            write_log("System", "EMERGENCY SHUTDOWN INITIATED");
                            broadcast("\n[Sistem] Server dimatikan secara darurat oleh The Knights.", sd);
                            exit(0);
                        }
                    }
```

Dalam kode ini dikhususkan buat admin yang bernama `The Knights`. Nah dalam kode ini ketika username yang diinput adalah the knights, dia akan meminta password dan bikin `is_admin` yang awalnya adalah 0 menjadi 1. Nah disini juga ada fungsi remote procedure call (RPC) khusus admin yang bisa memantau user, uptime, dan mematikan server. Dan terakhir

```c
else {
                        char msg[1100];
                        sprintf(msg, "[%s]: %s", clients[i].username, buffer);
                        broadcast(msg, sd);
                        
                        char log_msg[1100];
                        snprintf(log_msg, sizeof(log_msg), "[%s]: %s", clients[i].username, buffer);
                        write_log("User", log_msg);
                    }
                }
            }
        }
    }
    return 0;
}
```

Adalah kode kalau misal bukan admin, dia hanya mengirim chat biasa tersebut.

Untuk error handling sendiri:

```c
FILE *f = fopen("history.log", "a");
if (!f) return;
``` 

Ketika file tersebut gagal dibuka, dia akan mengembalikan hasilnya null tanpa bikin `segmentation fault`.

```c
if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("Bind failed"); return 1;
}
if (listen(server_fd, 10) < 0) {
    perror("Listen failed"); return 1;
}
```

Ketika port 8080 lagi tidak bisa dibuka (misal yang lama masih tidak dihapus) maka dia akan mengirim pesan errornya dan menutup servernya

```c
valread = read(sd, buffer, 1024);
if (valread == 0 || strcmp(buffer, EXIT_CMD) == 0) {
    close(sd);
    clients[i].socket = 0;
}
```

akan menghandle ketika client ini memutuskan diri dari koneksi server (contohnya: ctrl + c). Nah dia akan mengosongkan socketnya dan menutupnya.

```c
if (dup) send(sd, "ERR_DUP", 7, 0);
```

Ini akan mengecheck apakah namanya sama atau tidak. Jika sama akan mengirim errornya.

```c
buffer[strcspn(buffer, "\r\n")] = 0;
```

Ini hanya membersihkan carriage returnnya saja. Next filenya:

** navi.c **

Pertama-tama untuk setupnya seperti ini:

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "protocol.h"

int sock;
int running = 1;
int is_admin = 0; 
```

Dengan mengimport library yang dibutuhkan dan mendefinisikan running = 1 dan admin = 0. Lalu selanjutnya

```c
void *receive_msg(void *arg) {
    char buf[1024];
    while (running) {
        memset(buf, 0, 1024);
        if (read(sock, buf, 1024) > 0) {
            if (is_admin) {
                printf("\r\033[K%s\nPerintah >> ", buf);
            } else {
                printf("\r\033[K%s\n> ", buf);
            }
            fflush(stdout);
        } else {
            printf("\r\033[K\n[Sistem] Koneksi ke The Wired terputus.\n");
            running = 0;
            exit(0);
        }
    }
    return NULL;
}
```

adalah fungsi yang menerima pesan secara asinkron pada thread tersebut. Lalu pada fungsi main:

```c
int main() {
    struct sockaddr_in server;
    char username[50], buf[1024];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printf("Error pembuatan socket.\n"); return 1;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(WIRED_PORT);
    if (inet_pton(AF_INET, WIRED_IP, &server.sin_addr) <= 0) {
        printf("IP dari protocol.h tidak valid\n"); return 1;
    }

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        printf("[Sistem] Koneksi gagal! Pastikan server The Wired sudah menyala.\n"); 
        return 1;
    }
```

yaitu menjalankan fungsi setup koneksinya. Nah disini terdapat `inet_pton` yang mengubah string '127.0.0.1' menjadi angka binary yang dipahami oleh sistem dan menghubungkan dengan fungsi `connect` menuju alamt yang udah ditentukan pada protocol.h. Selanjutnya:

```c
while (1) {
        printf("Masukkan nama Anda: "); 
        
        fgets(username, 50, stdin);
        username[strcspn(username, "\r\n")] = 0; 
        
        if (strcmp(username, "The Knights") == 0) {
            printf("Masukkan Kata Sandi: ");
            char pass[50];
            fgets(pass, 50, stdin);
            pass[strcspn(pass, "\r\n")] = 0;
            
            sprintf(buf, "ADMIN:%s", pass);
        } else {
            sprintf(buf, "LOGIN:%s", username);
        }
        
        send(sock, buf, strlen(buf), 0);
        memset(buf, 0, 1024);
        
        int bytes_read = read(sock, buf, 1024);
        if (bytes_read <= 0) {
            printf("\n[Sistem] Gagal sinkronisasi, tidak ada respon dari server.\n");
            return 1;
        }
        
        if (strncmp(buf, "ERR_DUP", 7) == 0) {
            printf("[Sistem] Identitas '%s' sudah disinkronkan di The Wired.\n\n", username);
        } else if (strncmp(buf, "OK_ADMIN", 8) == 0) {
            printf("\n[Sistem] Otentikasi Berhasil. Hak akses Admin diberikan.\n");
            is_admin = 1;
            break;
        } else if (strncmp(buf, "OK", 2) == 0) {
            printf("\n--- Selamat datang di The Wired, %s\n ---", username);
            is_admin = 0;
            break;
        } else {
            printf("\n[Sistem] Autentikasi gagal. Password salah.\n\n");
        }
    }
```

nah pada program ini akan menjalankan identitas dari usernya. Ketika user identitasnya itu bukan `the knights` yang mana merupakan sebuah admin dalam soal ini, dia akan menjalankan socket sebagai user biasa. Tapi kalau sebagai `the knights` maka password akan muncul yang meminta player memasukkan passwordnya dan mendapatkan akses admin dengan identitas `the knights`. Lalu selanjutnya:

```c
pthread_t tid;
    pthread_create(&tid, NULL, receive_msg, NULL);

    while (running) {
        if (is_admin) {
            printf("\n=== KONSOL THE KNIGHTS ===\n");
            printf("1. Periksa Entitas Aktif (Pengguna)\n");
            printf("2. Periksa Waktu Aktif Server\n");
            printf("3. Lakukan Pemutusan Darurat\n");
            printf("4. Putuskan sambungan\n");
            printf("Perintah >> ");
            
            char choice_str[10];
            fgets(choice_str, 10, stdin);
            int choice = atoi(choice_str);
            
            if (choice == 1) send(sock, "CMD:USERS", 9, 0);
            else if (choice == 2) send(sock, "CMD:UPTIME", 10, 0);
            else if (choice == 3) {
                send(sock, "CMD:SHUTDOWN", 12, 0);
                running = 0;
            }
            else if (choice == 4) {
                printf("[Sistem] Memutuskan koneksi dari The Wired...\n");
                send(sock, EXIT_CMD, strlen(EXIT_CMD), 0);
                running = 0;
            }
            usleep(500000); 
        } else {
            printf("> ");
            fgets(buf, 1024, stdin);
            buf[strcspn(buf, "\r\n")] = 0; 
            
            if (strlen(buf) > 0) {
                if (strcmp(buf, "/KELUAR") == 0 || strcmp(buf, "/exit") == 0) {
                    printf("[Sistem] Memutuskan koneksi dari The Wired...\n");
                    send(sock, EXIT_CMD, strlen(EXIT_CMD), 0);
                    running = 0;
                } else {
                    send(sock, buf, strlen(buf), 0);
                }
            }
        }
    }

    close(sock);
    return 0;
}
```

akan dijalankan ketika di awal tadi pada saat fase identitas sudah selesai dijalankan maka akan menjalankan fungsi `pthread_create` yang membuat sebuah thread untuk dijalankan 2 jalur yaitu jalur untuk membaca pesannya dan jalur untuk membaca input dari user. Nah untuk kasus ini adalah kasus user biasa atau bisa dibilang bukan `the knights` itu sendiri. Untuk kasus admin atau `the knights` sendiri maka `is_admin` yang awalnya 0 menjadi 1 dan menjalankan konsol untuk muncul di permukaannya. Nah terdapat 4 pilihan dan 4 pilihan ini akan dimapping menjadi kode khusus seperti `CMD:USERS`nya itu. Nah disini ada fungsi `usleep` yang dimana mencegah race condition untuk mencegah menu uinya tidak tercetak sebelum balasan dari perintah si admin ini di eksekusi. 

** error handling pada navi.c **

Terdapat error handling di file ini yaitu: 

```c
sock = socket(AF_INET, SOCK_STREAM, 0);
if (sock < 0) {
    printf("Error pembuatan socket.\n"); 
    return 1; 
}
```

nah error handling pertama ini mengecheck apakah sistem operasinya ini bisa mengalokasikan memori yang sudah ditentukan atau tidak. Jika tidak, program tersebut akan diterminasi dengan cepat. Selanjutnya

```c
if (inet_pton(AF_INET, WIRED_IP, &server.sin_addr) <= 0) {
    printf("IP dari protocol.h tidak valid\n"); 
    return 1; 
}
```

adalah kode untuk mengecheck apakah IP yang diubah dari string menjadi biner dan di terjemahkan kembali menjadi string ini valid atau tidak. jika tidak, program akan diterminasi. Selanjutnya

```c
if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
    printf("[Sistem] Koneksi gagal! Pastikan server The Wired sudah menyala.\n"); 
    return 1; 
}
```

Nah dalam kode ini akan menjalankan pengechekan apakah program klien ini menjalankan deadlock atau tidak ketika terhubung ke sebuah port yang sudah ditentukan. Jika iya dan port itu belum jalan, program akan dimatikan agar tidak hang saat dijalankan. Lalu

```c
int bytes_read = read(sock, buf, 1024);
if (bytes_read <= 0) {
    printf("\n[Sistem] Gagal sinkronisasi, tidak ada respon dari server.\n");
    return 1; 
}
```

akan mengecheck apakah pada saat mengisi identitas ada respond dari host atau tidak. jika tidak, fungsi read akan menangkap sinyal itu dan program akan dimatikan. Selanjutnya

```c
if (strncmp(buf, "ERR_DUP", 7) == 0) {
    printf("[Sistem] Identitas '%s' sudah disinkronkan di The Wired.\n\n", username);
```

dan

```c
} else {
    printf("\n[Sistem] Autentikasi gagal. Password salah.\n\n");
```

adalah fungsi yang mengecheck apakah terdapat username yang sama atau tidak. Jika sama, akan meminta username yang baru dan ketika password. Dan terakhir:

```c
} else {

    printf("\r\033[K\n[Sistem] Koneksi ke The Wired terputus.\n");
    running = 0;  utama
    exit(0);
```

adlaah fungsi yang dimana dia akan memutus koneksi ketika server tidak terhubung dan akan mengeluarkan dari thread. 
#### output

1. Mengambil file zip

![alt text](/assets/soal_1/ambil%20zip.png)

2. Check kondisi dan menghapus file zip

![alt text](/assets/soal_1/unzip%20and%20delete.png)

3. Compile file `kenz_rescue.c`

![alt text](/assets/soal_1/compile.png)

4. Menjalankan file `kenz_rescue`

![alt text](/assets/soal_1/jalanin%20fuse.png)

5. Hasil 

![alt text](/assets/soal_1/result.png)

6. Hasil akhir

![alt text](/assets/soal_1/hasil_akhir.png)

#### Kendala

Tidak ada kendala
