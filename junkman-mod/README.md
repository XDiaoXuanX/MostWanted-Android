# Junkman & Marker Mod (NFS Most Wanted Android)

Modifikasi tidak resmi untuk [RanggaCasper/MostWanted-Android](https://github.com/RanggaCasper/MostWanted-Android) **v1.4.0**.
Proyek asli: lisensi **GPL-3.0**, dibangun di atas XboxXBE-Recomp (TGP482) dan host Android dari Black-Android.
Tanggal modifikasi: 2026-11-11 (patch biner pada `libmain.so`, daftar lengkap di `apply_patch.py`).
Folder ini hanya berisi skrip patch dan dokumentasi. **Tidak ada aset game di sini.**

> Unofficial mod. Not affiliated with or endorsed by Electronic Arts or the original author.
> Need for Speed is a trademark of Electronic Arts Inc. You need your own copy of the game.

## Apa yang diubah

| Varian | Isi |
|---|---|
| **A** | "Selections Remaining" 2 -> **6**. Marker tidak berkurang saat dipakai (Junkman tak habis). |
| **D** | A + stok semua jenis dianggap tersedia (x63) + gerbang Backroom dibuka, jadi semua kategori Backroom (Performance, Parts, Visual) tampil. **Direkomendasikan.** |
| **E** | D + lewati cek data Junkman per mobil untuk 5 jenis dasar. **Eksperimen, bisa crash.** |

Induction dan Nitrous tetap bergantung pada apakah mobil menerima kitnya.
Varian D sudah dicoba di Chevrolet Cobalt dan Mazda RX-8: Backroom menampilkan banyak kategori, dan Junkman tidak habis setelah dibeli.

## Cara membuat APK sendiri

1. Ambil APK resmi v1.4.0 dari halaman Releases proyek asli.
2. Ekstrak `lib/arm64-v8a/libmain.so` dari APK itu.
3. Jalankan:
   ```
   python apply_patch.py libmain.so libmain_patched.so --varian D
   ```
   Skrip memeriksa tiap instruksi sebelum mengubahnya. Kalau build berbeda, skrip berhenti tanpa mengubah apa pun.
4. Ganti file `libmain.so` di dalam APK dengan hasilnya (namanya harus tetap `libmain.so`), lalu tanda tangani ulang APK dengan kunci milikmu sendiri.
5. Karena tanda tangan berbeda dari APK resmi, hapus instalasi resmi dulu sebelum memasang APK ini. Backup folder `MostWanted/hdd` (save) lebih dulu.

Dites pada `libmain.so` dengan SHA-256 `b94016abbfe99a83f471e9322f3b7a62bfa0e95d9e34ee58a4300381d2d5bdd1`.

## Batasan yang diketahui

- Ini patch biner pada kode hasil rekompilasi, bukan perubahan di source. Hanya berlaku untuk build yang instruksinya sama.
- Menjalankan varian E tanpa backup save tidak disarankan.
- Daftar patch lengkap (offset, sebelum, sesudah) ada di `apply_patch.py`.

## Kredit & lisensi

Seluruh kredit proyek asli tetap berlaku: RanggaCasper (port Android), TGP482 (XboxXBE-Recomp), bagasromadon (desain tombol), SDL, glslang, toml++, Dear ImGui.
Proyek asli dirilis di bawah **GPL-3.0** (lihat file `LICENSE` di root repo ini; jangan dihapus atau diubah).
Modifikasi ini, termasuk skrip dan dokumentasi di folder ini, juga dirilis di bawah GPL-3.0.
Kalau kamu membagikan APK hasil modifikasi, sediakan juga source-nya (repo ini) dan catat perubahan yang kamu buat, seperti di README ini.
