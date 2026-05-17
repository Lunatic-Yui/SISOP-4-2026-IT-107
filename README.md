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

### Soal 2

#### Penjelasan

**server** dan **notes.csv.enc**

Diberikan link gdrive dan setelah dicheck, ada 2 file dalam gdrive itu. Saya menggunakan `gdown` untuk mendownload kedua file itu. Selanjutnya

**fuse.c**

Membuat fuse.c. Kode programnya seperti ini:

```c

#define FUSE_USE_VERSION 28
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <stdlib.h>

static char source_dir[1024];

#define XOR_KEY 0x76
```

adalah kode untuk setupan awalnya dengan mendefinisikan `XOR_KEY 0x76` dan kalau diartikan dalam 0x76 sendiri adalah 118. Dan juga terdapat source_dir yang memiliki batasnya adalah 1024 dengan library yang dibutuhkan. Selanjutnya

```c
static void xor_crypt(char *buf, size_t size) {
    for (size_t i = 0; i < size; i++) {
        buf[i] ^= XOR_KEY;
    }
}
```

adalah kode untuk membuat enkripsi dengan key `XOR_KEY` yang sudah di definisikan atau sudah di set. Selanjutnya

```c
static void get_dir_path(char *res, const char *path) {
    sprintf(res, "%s%s", source_dir, path);
}

static void get_file_path(char *res, const char *path) {
    sprintf(res, "%s%s.enc", source_dir, path);
}
```

Dalam kode ini, terdapat dua fungsi pendukung (helper functions) yang bertugas melakukan pemetaan alamat (path mapping) dari sistem berkas virtual ke lokasi penyimpanan fisik menggunakan fungsi manipulasi string `sprintf`.

Pertama, fungsi `get_dir_path` digunakan untuk memetakan path direktori dengan menggabungkan string dari direktori sumber (`source_dir`) dan path yang direquest (path) tanpa modifikasi tambahan.

Kedua, fungsi `get_file_path` digunakan secara spesifik untuk memetakan path dari sebuah file. Berbeda dengan direktori, fungsi ini secara otomatis menyisipkan ekstensi `.enc` di akhir path yang direquest. Hal ini dilakukan karena file secara fisik disimpan dalam format terenkripsi dengan ekstensi `.enc` di dalam direktori sumber, namun ditampilkan seolah-olah sebagai file normal tanpa ekstensi tersebut pada direktori mount (virtual point). Selanjutnya

```c
static int xmp_getattr(const char *path, struct stat *stbuf) {
    char fpath[1024];
    
    get_dir_path(fpath, path);
    if (lstat(fpath, stbuf) == 0) return 0;

    get_file_path(fpath, path);
    if (lstat(fpath, stbuf) == 0) return 0;

    return -ENOENT; 
}
```

Fungsi `xmp_getattr` bertugas untuk mengambil dan mengembalikan metadata dari sebuah file atau direktori. Pada implementasi ini, fungsi diatur untuk melakukan pengecekan ganda (dual-check) menggunakan system call `lstat` untuk mengatasi perbedaan penamaan antara virtual path dan physical path.

Pertama, program akan mengasumsikan path yang diminta sebagai direktori dan memetakan path-nya menggunakan `get_dir_path`. Jika `lstat` mengembalikan nilai keberhasilan (0), atribut akan langsung diteruskan. Jika gagal, program akan mengasumsikan path tersebut sebagai sebuah file dan memetakannya ulang menggunakan `get_file_path` (yang menyisipkan ekstensi `.enc`), kemudian kembali melakukan pengecekan dengan `lstat`. Apabila kedua tahap validasi ini gagal, fungsi akan mengembalikan error `-ENOENT` yang menandakan bahwa file atau direktori tersebut memang tidak eksis di dalam penyimpanan fisik. Selanjutnya

```c
static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    char fpath[1024];
    get_dir_path(fpath, path);

    DIR *dp = opendir(fpath);
    if (dp == NULL) return -errno;

    struct dirent *de;
    while ((de = readdir(dp)) != NULL) {
        char name[256];
        strcpy(name, de->d_name);

        if (de->d_type == DT_REG || de->d_type == DT_UNKNOWN) {
            size_t len = strlen(name);
            if (len > 4 && strcmp(name + len - 4, ".enc") == 0) {
                name[len - 4] = '\0';
            }
        }

        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        filler(buf, name, &st, 0);
    }
    closedir(dp);
    return 0;
}
```

Nah dalam kode ini, fungsi `xmp_readdir` bertugas untuk membaca dan menampilkan isi dari sebuah direktori (directory listing). Pada implementasi sistem berkas terenkripsi ini, fungsi `readdir` dimodifikasi untuk menyembunyikan ekstensi file fisik (`.enc`) dari antarmuka pengguna.

Program melakukan iterasi menggunakan `readdir` dan memfilter entri yang berupa file reguler (`DT_REG`). Pada file-file tersebut, dilakukan pengecekan panjang karakter dan pencocokan substring `".enc"` di akhir nama file menggunakan `strcmp`. Jika cocok, program memanipulasi string dengan menyisipkan karakter null terminator (`'\0'`) pada indeks ke `len - 4`. Teknik ini secara efektif memotong ekstensi `.enc` sebelum nama file diteruskan ke buffer FUSE menggunakan fungsi `filler`, sehingga pengguna hanya melihat nama file dengan ekstensi aslinya (contoh: `file.txt.enc` akan dirender sebagai `file.txt`). Kemudian

```c
static int xmp_mkdir(const char *path, mode_t mode) {
    char fpath[1024];
    get_dir_path(fpath, path);
    int res = mkdir(fpath, mode);
    if (res == -1) return -errno;
    return 0;
}

static int xmp_rmdir(const char *path) {
    char fpath[1024];
    get_dir_path(fpath, path);
    int res = rmdir(fpath);
    if (res == -1) return -errno;
    return 0;
}
```

adalah 2 kode yang mempunyai fungsi fundamental dalam manajemen direktori pada sistem berkas FUSE, yang masing-masing bertugas untuk membuat dan menghapus direktori kosong.

Kedua fungsi ini beroperasi menggunakan mekanisme passthrough murni. Karena skenario enkripsi ekstensi `.enc` hanya berlaku untuk file, kedua fungsi ini menggunakan helper `get_dir_path` untuk memetakan path virtual menjadi path fisik tanpa memodifikasi nama direktori. Setelah path fisik diperoleh, eksekusi operasi didelegasikan sepenuhnya kepada system call POSIX standar, yaitu `mkdir()` (dengan menyertakan parameter `mode` untuk permission atau Access Control List) dan `rmdir()`. Apabila terjadi penolakan atau kegagalan pada level sistem operasi (seperti direktori tidak kosong saat dihapus), fungsi akan meneruskan kode error bawaan sistem (`-errno`) kembali ke antarmuka pengguna FUSE.

```c
static int xmp_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    char fpath[1024];
    get_file_path(fpath, path); 
    int res = creat(fpath, mode);
    if (res == -1) return -errno;
    fi->fh = res;
    return 0;
}

static int xmp_open(const char *path, struct fuse_file_info *fi) {
    char fpath[1024];
    get_file_path(fpath, path);
    int res = open(fpath, fi->flags);
    if (res == -1) return -errno;
    close(res);
    return 0;
}
```

Merupakan dua kode krusial yang mengontrol gerbang akses direktori virtual, baik untuk inisiasi file baru maupun pembukaan file yang sudah ada. Mengingat objek yang ditangani pada tahap ini adalah sebuah file, kedua rutinitas ini mengawali prosedurnya dengan memanggil helper `get_file_path` guna memastikan ekstensi `.enc` tersisip dengan benar pada path fisik tujuan.

Meskipun memiliki mekanisme resolusi alamat yang sama, perlakuan terhadap file descriptor (ID referensi file) pada kedua fungsi ini memiliki perbedaan fundamental. Saat pengguna membuat file baru, rutinitas `xmp_create` mendelegasikan eksekusinya pada system call `creat()` dan secara persisten menyimpan file descriptor yang dihasilkan ke dalam struktur FUSE (`fi->fh`). Penyimpanan ini sangat krusial agar operasi penulisan (write) yang menyusul tidak kehilangan jejak file fisiknya. Sebaliknya, saat pengguna mengakses file yang sudah eksis, rutinitas `xmp_open` sekadar menguji hak akses melalui system call `open()` sesuai flags yang diminta sistem. Uniknya, apabila akses diizinkan, file descriptor tersebut justru seketika dilepaskan kembali melalui perintah `close()`. Pendekatan ini diterapkan karena pada arsitektur program ini, rutin pembukaan file difungsikan murni sebagai mekanisme validasi izin akses (permission check) sebelum sistem operasi benar-benar mengeksekusi proses pembacaan atau penulisan data di tahap selanjutnya. Lalu

```c

static int xmp_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    char fpath[1024];
    get_file_path(fpath, path);
    int fd = open(fpath, O_RDONLY);
    if (fd == -1) return -errno;

    int res = pread(fd, buf, size, offset);
    if (res > 0) xor_crypt(buf, res); 

    close(fd);
    if (res == -1) return -errno;
    return res;
}

static int xmp_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    char fpath[1024];
    get_file_path(fpath, path);
    int fd = open(fpath, O_WRONLY);
    if (fd == -1) return -errno;

    char *enc_buf = malloc(size);
    memcpy(enc_buf, buf, size);
    xor_crypt(enc_buf, size); 

    int res = pwrite(fd, enc_buf, size, offset);
    free(enc_buf);
    close(fd);
    if (res == -1) return -errno;
    return res;
}
```

Merupakan dua operasi utama pengelola lalu lintas data I/O yang mengawali prosedurnya dengan menyisipkan ekstensi `.enc` melalui rutinitas `get_file_path`. Pada operasi pembacaan (`xmp_read`), data mentah ditarik menggunakan system call `pread`, lalu seketika didekripsi di dalam buffer menggunakan fungsi `xor_crypt` secara on-the-fly sebelum ditampilkan ke pengguna. Sebaliknya, operasi penulisan (`xmp_write`) memerlukan manajemen memori khusus karena buffer input dari sistem bersifat read-only. Oleh karena itu, program mengalokasikan ruang memori sementara secara dinamis menggunakan `malloc` untuk menyalin dan mengenkripsi data sebelum dituliskan secara fisik melalui `pwrite`. Memori sementara ini kemudian wajib dibebaskan menggunakan perintah `free` guna mencegah terjadinya kebocoran memori (memory leak). Lalu

```c

static int xmp_truncate(const char *path, off_t size) {
    char fpath[1024];
    get_file_path(fpath, path);
    int res = truncate(fpath, size);
    if (res == -1) return -errno;
    return 0;
}

static int xmp_unlink(const char *path) {
    char fpath[1024];
    get_file_path(fpath, path);
    int res = unlink(fpath);
    if (res == -1) return -errno;
    return 0;
}

static int xmp_access(const char *path, int mask) {
    char fpath[1024];
    get_dir_path(fpath, path);
    if (access(fpath, mask) == 0) return 0;

    get_file_path(fpath, path);
    if (access(fpath, mask) == 0) return 0;

    return -ENOENT;
}
```

Nah pada ketiga program ini melengkapi fungsionalitas manajemen entitas dasar melalui pendelegasian langsung ke system call POSIX. Operasi pemotongan ukuran file (`xmp_truncate`) dan penghapusan file (`xmp_unlink`) secara spesifik menargetkan file fisik, sehingga keduanya menggunakan rutin `get_file_path` untuk menyisipkan ekstensi `.enc` sebelum mengeksekusi fungsi bawaan `truncate()` dan `unlink()`. Sementara itu, operasi `xmp_access` yang bertugas memvalidasi izin akses pengguna (seperti izin baca atau tulis) harus menerapkan mekanisme pengecekan ganda (dual-check fallback) layaknya operasi pengambilan atribut. Hal ini dikarenakan sistem harus menguji eksistensi dan hak akses target sebagai sebuah direktori terlebih dahulu melalui `get_dir_path`. Apabila uji pertama tersebut gagal, sistem akan berasumsi bahwa targetnya adalah sebuah file dan kembali melakukan validasi dengan menyisipkan ekstensi `.enc` melalui `get_file_path`, sebelum akhirnya mengembalikan error `-ENOENT` jika kedua validasi tersebut tidak membuahkan hasil. Kemudian

```c
static int xmp_rename(const char *from, const char *to) {
    char ffrom[1024], fto[1024];
    struct stat st;
    
    get_dir_path(ffrom, from);
    if (lstat(ffrom, &st) == 0 && S_ISDIR(st.st_mode)) {
        get_dir_path(fto, to); 
    } else {
        get_file_path(ffrom, from); 
        get_file_path(fto, to);
    }

    int res = rename(ffrom, fto);
    if (res == -1) return -errno;
    return 0;
}

static int xmp_utimens(const char *path, const struct timespec ts[2]) {
    char fpath[1024];
    struct stat st;

    get_dir_path(fpath, path);
    if (lstat(fpath, &st) != 0) {
        get_file_path(fpath, path);
    }

    struct timeval tv[2];
    tv[0].tv_sec = ts[0].tv_sec;
    tv[0].tv_usec = ts[0].tv_nsec / 1000;
    tv[1].tv_sec = ts[1].tv_sec;
    tv[1].tv_usec = ts[1].tv_nsec / 1000;

    int res = utimes(fpath, tv);
    if (res == -1) return -errno;
    return 0;
}
```

Untuk memodifikasi entitas dan metadatanya, kedua program ini menerapkan deteksi target secara adaptif. Pada kode `xmp_rename`, program memanfaatkan `lstat` untuk memeriksa sumber; direktori akan dipetakan secara normal, sedangkan target berupa file akan otomatis disisipkan ekstensi `.enc` pada path asal dan tujuan sebelum system call `rename()` dieksekusi. Sementara itu, operasi pembaruan waktu (`xmp_utimens`) menggunakan pengujian path serupa untuk menemukan lokasi fisiknya. Poin krusial pada operasi I/O ini terletak pada konversi resolusi waktu; struktur data FUSE yang berpresisi nanodetik (`timespec`) wajib dibagi 1000 untuk ditransformasi menjadi format mikrodetik (`timeval`) agar pembaruan metadata dapat diproses dengan valid oleh system call `utimes()`. Dan yang terakhir:

```c
static struct fuse_operations xmp_oper = {
    .getattr  = xmp_getattr,
    .readdir  = xmp_readdir,
    .mkdir    = xmp_mkdir,
    .rmdir    = xmp_rmdir,
    .create   = xmp_create,
    .open     = xmp_open,
    .read     = xmp_read,
    .write    = xmp_write,
    .truncate = xmp_truncate,
    .unlink   = xmp_unlink,
    .access   = xmp_access,
    .rename   = xmp_rename,
    .utimens  = xmp_utimens, 
};

int main(int argc, char *argv[]) {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd() error");
        return 1;
    }

    snprintf(source_dir, sizeof(source_dir), "%s/encrypted_storage", cwd);
    mkdir(source_dir, 0777); 

    char mount_dir[1024];
    snprintf(mount_dir, sizeof(mount_dir), "%s/fuse_mount", cwd);
    mkdir(mount_dir, 0777); 

    char *fuse_argv[] = { argv[0], mount_dir };
    int fuse_argc = 2;

    return fuse_main(fuse_argc, fuse_argv, &xmp_oper, NULL);
}
```

Bagian ini terdiri dari dua komponen konfigurasi utama. Struktur `fuse_operations` bertindak sebagai tabel pemetaan (_dispatch table_) esensial yang meregistrasikan seluruh rutinitas kustom—mulai dari manajemen I/O hingga modifikasi metadata agar dapat dikenali dan dieksekusi secara dinamis oleh library FUSE. Sementara itu, fungsi main mengambil alih inisiasi lingkungan secara otomatis. Alih-alih bergantung pada argumen manual pengguna, program secara mandiri mendeteksi direktori kerja aktif (`getcwd`) untuk membangun direktori sumber (`encrypted_storage`) beserta titik kaitnya (`fuse_mount`) melalui system call `mkdir`. Setelah infrastruktur folder dipastikan siap, siklus hidup program diserahkan sepenuhnya kepada `fuse_main` untuk menjalankan daemon sistem berkas di latar belakang berbekal tabel pemetaan yang telah dikonfigurasi.

**Dockerfile

Untuk program Dockerfilenya sendiri seperti ini:

```Dockerfile
FROM ubuntu:latest

WORKDIR /app

COPY server /app/

EXPOSE 9000

CMD ["./server"]
```

Ini adalah setupun untuk docker. Pertama adalah menentukan perangkat sandbox terlebih dahulu. Dalam soal diminta `ubuntu:latest` yang artinya sandboxnya harus ubuntu terbaru. Selanjutnya `WORKDIR` yang menuju ke app. Nah workdir ini sendiri adlaah command untuk working directorynya. Lalu `COPY` adalah perintah mengcopy file server ke dalam file `/app/` ini. Selanjutnya `EXPOSE` adalah port yang ke ngeexpose port berapa. Dalam konteks ini, port yang harus di expose adalah 9000. Dan terakhir `CMD` adalah command untuk menjalankan filenya.

**client.c**

Untuk kode programnya:

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 9000
#define BUFFER_SIZE 1024
```

Adalah program setup-an awalnya dengan mendefinisikan portnya 9000 dan buffer_sizenya 1024. Selanjutnya:

```c
int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    char command[BUFFER_SIZE];

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed. Pastikan Docker server sudah jalan! \n");
        return -1;
    }

    printf("Connected to DB Server on port %d\n", PORT);
    printf("Type HELP for available commands\n");
    printf("Type EXIT to quit\n");

    while (1) {
        printf("\ndb > ");
        fgets(command, BUFFER_SIZE, stdin);
        command[strcspn(command, "\n")] = 0; 

        if (strcmp(command, "EXIT") == 0) {
            break;
        }

        send(sock, command, strlen(command), 0);
        
        memset(buffer, 0, BUFFER_SIZE);
        int valread = read(sock, buffer, BUFFER_SIZE);
        if (valread > 0) {
            printf("%s\n", buffer);
        }
    }

    close(sock);
    return 0;
}
```

Nah pada kode main ini sebagai aspek komunikasi antar-proses (IPC), blok kode ini mengimplementasikan aplikasi klien berbasis socket TCP/IP yang bertindak sebagai antarmuka interaktif menuju database server. Fase inisiasi dimulai dengan pembentukan socket berjenis `SOCK_STREAM` (TCP) yang dikonfigurasi untuk menyambung ke antarmuka localhost (`127.0.0.1`). Transformasi alamat IP dari format teks menjadi bentuk biner ditangani secara aman oleh utilitas `inet_pton`, sebelum system call `connect()` memicu prosedur Three-Way Handshake untuk membangun sesi komunikasi yang reliabel dengan server.

Setelah koneksi terjalin, alur eksekusi bertransisi ke dalam siklus looping tak terbatas (CLI mandiri). Pada fase ini, program menangkap instruksi pengguna melalui `fgets`, mensterilkan karakter newline bawaannya, dan mentransmisikan payload tersebut ke server via perintah `send()`. Siklus ini akan langsung tertahan (blocking) pada operasi `read()` untuk menunggu dan mencetak respons balik dari server ke layar terminal. Interaksi dua arah ini akan terus berlangsung hingga pengguna memberikan interupsi berupa perintah 'EXIT', yang secara elegan akan menghentikan perulangan dan membebaskan sumber daya jaringan melalui rutinitas `close()`.

#### output

1. Mengambil file dari gdrive yang diberikan

![alt text](/assets/soal_2/mengambil%20file.png)

2. Compile `fuse.c`, `client.c` dan build Dockerfilenya

![alt text](/assets/soal_2/build%20docker%20and%20compile%20file%20c.png)

3. Menjalankan fusenya

![alt text](/assets/soal_2/jalanin%20fuse.png)

4. Mencoba membuat file txt dan menghapus semua filenya

![alt text](/assets/soal_2/percobaan%20file.png)

5. Membandingkan isi file pada folder `fuse_mount` dan `encrypted_storage`

![alt text](/assets/soal_2/encrypt-file.png)

6. Check list docker images

![alt text](/assets/soal_2/Docker%20images.png)

7. Membuat database dan tabelnya dan menunjukkan hasilnya

![alt text](/assets/soal_2/Docker.png)

#### Kendala

Kendalanya: berantem sama command docker yang tidak nembus ke encrypted_storage dan fuse_mount :>. Selain itu kendala dalam encrypt karena dalam contohnya ada tanda belah ketupa + `?` sedangkan punya saya hanya `v`. Itu saja

#### Klarifikasi

Jika dalam video no 2 itu tidak bisa automatic bikin foldernya, itu salah saya sendiri karena tidak melihat `mkdir` dalam soalnya. Itu saja

### Soal 3

#### Penjelasan

**Dockerfile**

Pertama-tama membuat file Dockerfilenya terlebih dahulu

```Dockerfile
FROM ubuntu:latest

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y samba && apt-get clean

WORKDIR /app

COPY . /app/

RUN cp /app/smb.conf /etc/samba/smb.conf

ENTRYPOINT ["/bin/bash", "/app/entrypoint.sh"]
```

Nah dalam kode ini ada beberapa: `FROM` untuk mendefinisikan sistem sandbox, `ENV` sebagai environtment yang user tuju, `RUN apt get-update...` untuk menjalankan programnya yang dijalankan, `WORKDIR` tempat direktori bekerja, `COPY . /app/` untuk mengcopy semua file ke dalam `/app/`, `RUN cp /app/smb.conf ...` untuk menjalankan programnya dengan mengcopy, dan `ENTRYPOINT` buat menjalankan file-filenya. Selanjutnya

**smb.conf**

```conf
[global]
    workgroup = WORKGROUP
    server string = LibraryIT Server
    security = user
    map to guest = bad user
    dns proxy = no

    vfs objects = full_audit
    full_audit:prefix = AUDIT|%u|%S
    full_audit:success = connect pwrite
    full_audit:failure = connect pwrite
    full_audit:facility = local7
    full_audit:priority = notice
```

Untuk configurasi awalnya secara global. Selanjutnya

```conf
[ebooks]
    path = /libraryit/ebooks
    browseable = yes
    guest ok = yes
    read only = no

[papers]
    path = /libraryit/papers
    browseable = yes
    guest ok = yes
    read only = no

[docs]
    path = /libraryit/docs
    browseable = yes
    guest ok = yes
    read only = no
    force user = root

[SourceCode]
    path = /libraryit/sourcecode
    browseable = no
    valid users = @staff
    read only = no
```

adalah configurasi untuk masing-masing user dengan permissionnya. Kemudian file:

**entrypoint.sh**

Untuk kode programnya:

```sh
if ! command -v rsyslogd &> /dev/null; then
    apt-get update && apt-get install -y rsyslog samba-vfs-modules
fi
```

adalah awal pengecekan pada lingkungan docker container ini. Nantinya `rsyslog` sendiri sebagai tempat penyimpanan virtual tanpa memunculkan errornya di terminal. Selanjutnya:

```sh
mkdir -p /var/log/samba
touch /var/log/samba/raw.log
touch /var/log/samba/libraryit.log
chmod 777 /var/log/samba/raw.log /var/log/samba/libraryit.log

echo "local7.* /var/log/samba/raw.log" > /etc/rsyslog.d/samba-audit.conf
service rsyslog restart

mkdir -p /libraryit/ebooks /libraryit/papers /libraryit/sourcecode /libraryit/docs
```

adalah command-command setupun yang dibutuhkan sebelum dipakai oleh user seperti menyiadakan tempat folder ebooks, papers, dkk. Dan juga menyiapkan untuk logs sebagai pencatat untuk `docker logs` nantinya. Selanjutnya

```sh
userdel $(id -un 1000) 2>/dev/null
groupdel $(getent group 50 | cut -d: -f1) 2>/dev/null
groupdel $(getent group 100 | cut -d: -f1) 2>/dev/null

groupadd -g 50 staff
groupadd -g 100 readonly

useradd -M -u 1000 -s /sbin/nologin -c "" member
(echo "member123"; echo "member123") | smbpasswd -a -s member

useradd -M -u 1001 -s /sbin/nologin -c "" contributor
(echo "contrib456"; echo "contrib456") | smbpasswd -a -s contributor

useradd -M -u 1002 -s /sbin/nologin -c "" librarian
(echo "lib789"; echo "lib789") | smbpasswd -a -s librarian

usermod -aG staff librarian
usermod -aG staff contributor
usermod -aG readonly member

chown -R root:staff /libraryit
chmod 775 /libraryit/ebooks /libraryit/papers /libraryit/docs
chmod 750 /libraryit/sourcecode

smbd -F -d 2
```

adalah program untuk menentukan identitas user beserta password dan permissionnya. Nah dalam program ini dikelompokkan ke beberapa seperti member ke kelompok member, contributor ke kelompok contributor, dan librarian ke kelompok librarian. Nah masing-masing kelompok ini mempunyai passwordnya sendiri dan permissionnya sendiri. Selanjutnya file:

**docker-compose.yml**

```yml
services:
  libraryit:
    build: .
    container_name: libraryit-server
    ports:
      - "1445:445"
      - "1139:139"
    volumes:
      - ./data:/libraryit
      - shared_logs:/var/log/samba
    restart: unless-stopped

  libraryit-logger:
    image: debian:latest
    container_name: libraryit-logger
    volumes:
      - shared_logs:/var/log/samba
    depends_on:
      - libraryit
    command:
      - /bin/bash
      - -c
      - |
        tail -F /var/log/samba/raw.log | while read -r line; do
          if [[ "$$line" == *"AUDIT|"* ]]; then
            data="$${line#*AUDIT|}"
            IFS='|' read -r user share op status file <<< "$$data"
            if [[ "$$status" == *"fail"* ]]; then
              lvl="WARNING"
              act="DENIED"
              target="$$share"
            else
              lvl="INFO"
              if [ "$$op" = "connect" ]; then 
                act="CONNECT"
                target="$$share"
              elif [ "$$op" = "pwrite" ]; then 
                act="WRITE"
                target="$${file##*/}"
              else 
                act="$$op"
                target="$$file"
              fi
            fi
            dt=$$(date '+%Y-%m-%d %H:%M:%S')
            
            echo "[$$dt] [$$lvl] [$$user] [$$act] [$$target]" >> /var/log/samba/libraryit.log
            echo "[$$dt] [$$lvl] [$$user] [$$act] [$$target]"
          fi
        done

volumes:
  shared_logs:
```

adalah kode yang menyalakan program dengan attach ke `Dockerfilenya` sekaligus mencatatnya ke lognya ketika user menggunakan command `Docker logs -f libraryit-logger`. Namun ada kode yang diperbaikin dan versi diperbaikinnya:

```yml
services:
  libraryit:
    build: .
    container_name: libraryit-server
    ports:
      - "1445:445"
      - "1139:139"
    volumes:
      - ./data:/libraryit
      - shared_logs:/var/log/samba   
    restart: unless-stopped

  libraryit-logger:
    image: debian:latest
    container_name: libraryit-logger
    volumes:
      - shared_logs:/var/log/samba   
      - ./logs:/app/logs            
    depends_on:
      - libraryit
    command:
      - /bin/bash
      - -c
      - |
        mkdir -p /app/logs
        
        tail -F /var/log/samba/raw.log | while read -r line; do
          if [[ "$$line" == *"AUDIT|"* ]]; then
            data="$${line#*AUDIT|}"
            IFS='|' read -r user share op status file <<< "$$data"
            if [[ "$$status" == *"fail"* ]]; then
              lvl="WARNING"
              act="DENIED"
              target="$$share"
            else
              lvl="INFO"
              if [ "$$op" = "connect" ]; then 
                act="CONNECT"
                target="$$share"
              elif [ "$$op" = "pwrite" ]; then 
                act="WRITE"
                target="$${file##*/}"
              else 
                act="$$op"
                target="$$file"
              fi
            fi
            dt=$$(date '+%Y-%m-%d %H:%M:%S')
            
            echo "[$$dt] [$$lvl] [$$user] [$$act] [$$target]" >> /app/logs/libraryit.log
            echo "[$$dt] [$$lvl] [$$user] [$$act] [$$target]"
          fi
        done

volumes:
  shared_logs:
```

#### Higlight problem

Nah karena yang saya kumpulkan versi pertamanya jadi saya mau higlight beberapa kesalahannya (kesalahan yang dimaksud: tidak muncul text dalam log).

1. Volume hanya 1

```yml
volumes:
      - shared_logs:/var/log/samba
```

Nah dalam program docker-compose ini hanya terdapat 1 perintah menyimpan lognya yaitu `log/samba` saja. Kalau hanya 1, berarti semua isi log akan tersimpan dalam `log/samba` saja.

2. Tidak ada perintah `mkdir`

Nah kesalah kedua tidak membuat perintah `mkdir` pada command di docker command karena `mkdir` sendiri adalah `make directory` yang artinya membuat folder itu sendiri. 

3. Pembuangan `echo` ke `log/samba` bukan `logs/libraryit.log`

Programnya:

```yml
echo "[$$dt] [$$lvl] [$$user] [$$act] [$$target]" >> /var/log/samba/libraryit.log
```

Jadi pada perintah ini hanya terbuang pada root `var` dan lognya tersimpan dalam var itu. Akibatnya `libraryit.log` tidak muncul dalam `logs` yang user tempat sekarang. 

#### perbaikan

Untuk perbaikannya:

1. Menambahkan volume terpisah `logs` programnya:

```yml
    volumes:
      - shared_logs:/var/log/samba   
      - ./logs:/app/logs # -> add this one on the volumes karena pertama gk ada.
```

2. Menambahkan command `mkdir`, progamnya

```yml
command:
      - /bin/bash
      - -c
      - |
        mkdir -p /app/logs # -> command buat nambahkan directory
```

3. Menargetkan ke `/app/logs` bukan ke `var`. Kodenya:

```yml
echo "[$$dt] [$$lvl] [$$user] [$$act] [$$target]" >> /app/logs/libraryit.log
```

#### output

1. Docker compose up

![alt text](/assets/soal_3/compose%20up.png)

2. Result identity dan result yang lain

![alt text](/assets/soal_3/identity%20and%20result.png)

3. Testing permission

![alt text](/assets/soal_3/testing-perm.png)

4. Output yang lain untuk testing

![alt text](/assets/soal_3/output%20lain.png)

5. Hasil log dari semua kegiatan yang dilakukan

![alt text](/assets/soal_3/Hasil%20log.png)

#### Kendala

Tidak muncul isi logsnya. Perbaikannya sudah sebelum section `output`. Sisanya tidak ada.
