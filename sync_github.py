# -*- coding: utf-8 -*-
"""
3dbrint Farm - Otomatik Derleme, Paketleme ve GitHub Senkronizasyon Araci
Tüm 6 surumu ayri ayri derler, OTA ve Hizli Flasher ikililerini olusturur,
version.json gunceller ve GitHub reposuna push eder.
"""

import os
import sys
import shutil
import subprocess
import json
import datetime
import tempfile

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
FLASHER_DIR = os.path.abspath(os.path.join(BASE_DIR, "..", "NerdMiner_Hizli_Flasher"))
BIN_OTA_DIR = os.path.join(BASE_DIR, "bin")

ARDUINO_CLI = r"C:\Users\prome\AppData\Local\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
ESPTOOL = r"C:\Users\prome\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\5.3.1\esptool.exe"

VARIANTS = [
    {
        "key": "esp32c3_oled",
        "name": "ESP32-C3 Supermini (0.96 OLED)",
        "chip": "esp32c3",
        "fqbn": "esp32:esp32:esp32c3:PartitionScheme=min_spiffs",
        "profile_id": 1,
        "profile_name": "oled",
        "boot_offset": "0x0"
    },
    {
        "key": "esp32c3_headless",
        "name": "ESP32-C3 Supermini (Ekransiz / LED)",
        "chip": "esp32c3",
        "fqbn": "esp32:esp32:esp32c3:PartitionScheme=min_spiffs",
        "profile_id": 2,
        "profile_name": "headless",
        "boot_offset": "0x0"
    },
    {
        "key": "esp32c3_butonlu",
        "name": "ESP32-C3 Supermini (0.96 OLED + Dokunmatik)",
        "chip": "esp32c3",
        "fqbn": "esp32:esp32:esp32c3:PartitionScheme=min_spiffs",
        "profile_id": 3,
        "profile_name": "butonlu",
        "boot_offset": "0x0"
    },
    {
        "key": "esp32_oled",
        "name": "ESP32 DevKit v1 Dual-Core (0.96 OLED)",
        "chip": "esp32",
        "fqbn": "esp32:esp32:esp32:PartitionScheme=min_spiffs",
        "profile_id": 1,
        "profile_name": "oled",
        "boot_offset": "0x1000"
    },
    {
        "key": "esp32_headless",
        "name": "ESP32 DevKit v1 Dual-Core (Ekransiz / LED)",
        "chip": "esp32",
        "fqbn": "esp32:esp32:esp32:PartitionScheme=min_spiffs",
        "profile_id": 2,
        "profile_name": "headless",
        "boot_offset": "0x1000"
    },
    {
        "key": "esp32_butonlu",
        "name": "ESP32 DevKit v1 Dual-Core (0.96 OLED + Buton)",
        "chip": "esp32",
        "fqbn": "esp32:esp32:esp32:PartitionScheme=min_spiffs",
        "profile_id": 3,
        "profile_name": "butonlu",
        "boot_offset": "0x1000"
    }
]

def update_build_config(profile_id):
    cfg_file = os.path.join(BASE_DIR, "build_config.h")
    content = f"""#ifndef _BUILD_CONFIG_H_
#define _BUILD_CONFIG_H_

#define BUILD_PROFILE_OLED      1
#define BUILD_PROFILE_HEADLESS  2
#define BUILD_PROFILE_BUTONLU   3

// Otomatik derleme sistemi tarafindan dinamik ayarlanmistir:
#define CURRENT_BUILD_PROFILE   {profile_id}

#endif // _BUILD_CONFIG_H_
"""
    with open(cfg_file, "w", encoding="utf-8") as f:
        f.write(content)

def run_command(cmd, cwd=None):
    print(f"[*] Calistiriliyor: {cmd}")
    res = subprocess.run(cmd, shell=True, cwd=cwd or BASE_DIR, capture_output=True, text=True)
    if res.returncode != 0:
        print(f"[HATA] Komut basarisiz oldu (Kod: {res.returncode}):")
        if res.stdout:
            print(res.stdout[-1000:])
        if res.stderr:
            print(res.stderr[-1000:])
        return False
    return True

def compile_variant(v):
    print("\n" + "="*65)
    print(f"[*] DERLENIYOR: {v['name']} [{v['key']}]")
    print("="*65)

    update_build_config(v["profile_id"])

    with tempfile.TemporaryDirectory() as tmp_out:
        cmd = f'"{ARDUINO_CLI}" compile --fqbn {v["fqbn"]} --output-dir "{tmp_out}" "{BASE_DIR}"'
        if not run_command(cmd):
            print(f"[!] {v['key']} DERLEMESI BASARISIZ!")
            return False

        app_bin = os.path.join(tmp_out, "nerdminer.ino.bin")
        boot_bin = os.path.join(tmp_out, "nerdminer.ino.bootloader.bin")
        part_bin = os.path.join(tmp_out, "nerdminer.ino.partitions.bin")

        if not os.path.exists(app_bin):
            print(f"[!] nerdminer.ino.bin bulunamadi: {tmp_out}")
            return False

        # 1. GitHub OTA Klasorune Kopyala
        os.makedirs(BIN_OTA_DIR, exist_ok=True)
        dest_ota_app = os.path.join(BIN_OTA_DIR, f"nerdminer_{v['key']}.bin")
        shutil.copy2(app_bin, dest_ota_app)
        size_kb = os.path.getsize(dest_ota_app) / 1024.0
        print(f"[+] OTA Binary Hazir: {dest_ota_app} ({size_kb:.1f} KB)")

        # 2. Hizli Flasher Klasorune Kopyala
        dest_flasher_dir = os.path.join(FLASHER_DIR, "bin", v["chip"], v["profile_name"])
        os.makedirs(dest_flasher_dir, exist_ok=True)

        shutil.copy2(app_bin, os.path.join(dest_flasher_dir, "nerdminer.ino.bin"))
        if os.path.exists(boot_bin):
            shutil.copy2(boot_bin, os.path.join(dest_flasher_dir, "nerdminer.ino.bootloader.bin"))
        if os.path.exists(part_bin):
            shutil.copy2(part_bin, os.path.join(dest_flasher_dir, "nerdminer.ino.partitions.bin"))

        # boot_app0.bin dosyasini da kopyala
        boot_app0_src = os.path.join(FLASHER_DIR, "bin", "esp32c3", "headless", "boot_app0.bin")
        boot_app0_dst = os.path.join(dest_flasher_dir, "boot_app0.bin")
        if os.path.exists(boot_app0_src) and not os.path.exists(boot_app0_dst):
            shutil.copy2(boot_app0_src, boot_app0_dst)

        # Merged Binary Olustur (Hizli tek dosya flaslama icin)
        if os.path.exists(boot_bin) and os.path.exists(part_bin):
            merged_dst = os.path.join(dest_flasher_dir, "nerdminer.ino.merged.bin")
            boot_app0_file = boot_app0_dst if os.path.exists(boot_app0_dst) else boot_app0_src
            
            # merge_bin komutu
            boot_offset = v["boot_offset"]
            merge_cmd = (
                f'{ESPTOOL} --chip {v["chip"]} merge_bin -o "{merged_dst}" --fill-flash-size 4MB '
                f'{boot_offset} "{boot_bin}" 0x8000 "{part_bin}" 0xe000 "{boot_app0_file}" 0x10000 "{app_bin}"'
            )
            run_command(merge_cmd)

        print(f"[+] Flasher Paketi Hazir: {dest_flasher_dir}")
        return True

def update_version_manifest():
    vfile = os.path.join(BASE_DIR, "version.json")
    today = datetime.date.today().isoformat()
    data = {
        "version": "2.2.0",
        "name": "3dbrint Farm v2.2.0",
        "release_date": today,
        "release_notes": "Tek merkezli 3dbrint Farm mimarisi, Master 2s LED nabiz kalp atisi, cift cekirdek ESP32 DevKit v1 tam destegi, Zero-Touch ESP-NOW WiFi esleme ve tek tikla GitHub OTA guncellemesi.",
        "firmware": {
            "esp32c3_oled": "bin/nerdminer_esp32c3_oled.bin",
            "esp32c3_headless": "bin/nerdminer_esp32c3_headless.bin",
            "esp32c3_butonlu": "bin/nerdminer_esp32c3_butonlu.bin",
            "esp32_oled": "bin/nerdminer_esp32_oled.bin",
            "esp32_headless": "bin/nerdminer_esp32_headless.bin",
            "esp32_butonlu": "bin/nerdminer_esp32_butonlu.bin",
            "default": "bin/nerdminer_esp32c3_headless.bin"
        },
        "variants": {}
    }
    for v in VARIANTS:
        data["variants"][v["key"]] = {
            "name": v["name"],
            "chip": v["chip"],
            "display": v["profile_name"],
            "url": f"https://raw.githubusercontent.com/mefeoku/NerdMiner_Farm/main/bin/nerdminer_{v['key']}.bin"
        }

    with open(vfile, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2, ensure_ascii=False)
    print(f"\n[+] Manifest guncellendi: {vfile}")

def git_sync_and_push(commit_msg=None):
    print("\n" + "="*65)
    print("[*] GITHUB REPO SENKRONIZASYONU")
    print("="*65)

    msg = commit_msg or f"Update 3dbrint Farm v2.2.0 ({datetime.datetime.now().strftime('%Y-%m-%d %H:%M')})"
    
    run_command("git config user.name \"mefeoku\"", cwd=BASE_DIR)
    run_command("git config user.email \"promefe46@gmail.com\"", cwd=BASE_DIR)
    run_command("git add .", cwd=BASE_DIR)
    run_command(f'git commit -m "{msg}"', cwd=BASE_DIR)

    # Remote kontrolu
    res = subprocess.run("git remote -v", shell=True, cwd=BASE_DIR, capture_output=True, text=True)
    if "origin" not in res.stdout:
        print("[!] GitHub 'origin' baglantisi tanimli degil. Ekleniyor...")
        run_command("git remote add origin https://github.com/mefeoku/NerdMiner_Farm.git", cwd=BASE_DIR)
        run_command("git branch -M main", cwd=BASE_DIR)

    print("[*] GitHub'a push ediliyor (git push origin main)...")
    push_res = subprocess.run("git push -u origin main", shell=True, cwd=BASE_DIR, capture_output=True, text=True)
    if push_res.returncode == 0:
        print("[BASARILI] Tum surumler ve ikili dosyalar GitHub'a yuklendi!")
    else:
        print("[BILGI] Git push ciktisi:")
        print(push_res.stdout)
        print(push_res.stderr)
        print("[IPUCU] Eger repo henuz olusturulmadiysa, https://github.com/new adresinden 'NerdMiner_Farm' adinda bir repo olusturup calistirabilirsiniz.")

def main():
    print("""
====================================================================
    3dbrint Farm - Tum Surumleri Derleme ve GitHub Senkronizasyonu
====================================================================
    """)
    success_count = 0
    for v in VARIANTS:
        if compile_variant(v):
            success_count += 1
        else:
            print(f"[!] {v['name']} atlandi veya hata olustu.")

    # build_config.h dosyasini varsayilan OLED'e geri dondur
    update_build_config(1)
    update_version_manifest()

    print(f"\n[*] Tamamlanan derleme: {success_count}/{len(VARIANTS)}")

    if success_count > 0:
        git_sync_and_push()

    print("""
====================================================================
    SENKRONIZASYON ISLEMI TAMAMLANDI!
====================================================================
    """)

if __name__ == "__main__":
    main()
