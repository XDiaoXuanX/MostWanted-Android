#!/usr/bin/env python3
"""
Lisensi: GPL-3.0 (sama dengan proyek induk). Tanggal modifikasi: 2026-11-11.

Junkman / Marker mod untuk NFS Most Wanted Android (RanggaCasper/MostWanted-Android v1.4.0).

Skrip ini menerapkan beberapa patch 4-byte pada lib/arm64-v8a/libmain.so milik APK RESMI.
Skrip HANYA bekerja pada build yang isi instruksinya persis sama (tiap patch memverifikasi
kata aslinya dulu), jadi aman: kalau build berbeda, skrip berhenti tanpa mengubah apa pun.

Pemakaian:
    python apply_patch.py libmain.so libmain_patched.so --varian D

Varian:
    A  Pilihan marker 2 -> 6, marker tidak berkurang saat dipakai (Junkman tak habis).
    D  A + stok semua jenis dianggap tersedia + gerbang Backroom terbuka (semua kategori tampil).
    E  D + lewati cek data Junkman per mobil untuk 5 jenis dasar (EKSPERIMEN, bisa crash).
"""
import argparse, hashlib, struct, sys

NOP = 0xD503201F

# (offset di libmain.so, kata asli, kata baru, grup, keterangan)
PATCHES = [
    (0x14AA69C, 0x7100051F, 0x7100151F, "M", "layar marker: batas pilih #1 (cmp #1 -> #5)"),
    (0x14AACB0, 0x7100093F, 0x7100193F, "M", "layar marker: batas pilih #2 (cmp #2 -> #6)"),
    (0x1480058, 0x5280004B, 0x528000CB, "M", "teks Selections Remaining #1: 2 -> 6"),
    (0x1480068, 0x5280004A, 0x528000CA, "M", "teks Selections Remaining (0 terpilih): 2 -> 6"),
    (0x1480248, 0x5280004B, 0x528000CB, "M", "teks Selections Remaining #2: 2 -> 6"),
    (0xE53F4C, 0xB82D483F, NOP, "R", "RemoveMarker: id marker tidak dihapus"),
    (0xE53F58, 0xB82A483F, NOP, "R", "RemoveMarker: flag marker tidak dihapus"),
    (0xE54784, 0x540001A1, NOP, "C", "CountMarkers: lewati cek id (record 1)"),
    (0xE547AC, 0x54000061, NOP, "C", "CountMarkers: lewati cek flag (record 1)"),
    (0xE547C4, 0x540001C1, NOP, "C", "CountMarkers: lewati cek id (record 2)"),
    (0xE547F0, 0x54000061, NOP, "C", "CountMarkers: lewati cek flag (record 2)"),
    (0xE5480C, 0x540001C1, NOP, "C", "CountMarkers: lewati cek id (record 3)"),
    (0xE54838, 0x54000061, NOP, "C", "CountMarkers: lewati cek flag (record 3)"),
    (0xE906A0, 0x54000141, NOP, "G", "gerbang Backroom: lewati cek id"),
    (0xE906B0, 0x540000C1, NOP, "G", "gerbang Backroom: lewati cek arg"),
    (0xE906C4, 0x540002E0, 0x14000017, "G", "gerbang Backroom: flag -> selalu ketemu"),
    (0x111F9C4, 0x54000761, 0x54001FA1, "S", "dukungan mobil: lewati cek data untuk 5 jenis dasar"),
]
VARIAN = {"A": "MR", "D": "MRCG", "E": "MRCGS"}

def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("masuk", help="libmain.so asli dari APK resmi v1.4.0 (arm64-v8a)")
    ap.add_argument("keluar", help="nama file hasil patch")
    ap.add_argument("--varian", choices=sorted(VARIAN), default="D")
    a = ap.parse_args()

    data = bytearray(open(a.masuk, "rb").read())
    print("SHA-256 masukan:", hashlib.sha256(data).hexdigest())
    grup = VARIAN[a.varian]
    terpakai = [p for p in PATCHES if p[3] in grup]
    for off, lama, baru, g, ket in terpakai:
        if off + 4 > len(data) or struct.unpack_from("<I", data, off)[0] != lama:
            sys.exit(f"BERHENTI: isi 0x{off:x} tidak cocok ({ket}). Build ini berbeda; tidak ada yang diubah.")
    for off, lama, baru, g, ket in terpakai:
        struct.pack_into("<I", data, off, baru)
        print(f"  [{g}] 0x{off:x}: {lama:08x} -> {baru:08x}  {ket}")
    open(a.keluar, "wb").write(data)
    print("Selesai. SHA-256 hasil:", hashlib.sha256(data).hexdigest())

if __name__ == "__main__":
    main()
