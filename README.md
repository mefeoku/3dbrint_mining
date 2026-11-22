# ⚡ 3dbrint Farm - Centralized NerdMiner OS (v2.2.0)

<p align="center">
  <img src="media/images_128_128.h" width="100" alt="3dbrint Logo" style="display:none;" />
  <b>Merkezi Yönetimli, Otomatik WiFi Yedeklemeli, GitHub OTA ve Termal Korumalı ESP32 / ESP32-C3 NerdMiner Farm İşletim Sistemi</b>
</p>

---

## 🚀 Öne Çıkan Özellikler

* 🌐 **Tek Merkezli Farm Kontrol Paneli (`http://3dbrint.local`):**
  * Ağdaki tüm cihazlar birbirini otomatik tanır.
  * Tek bir cihaz **MASTER** seçilir, diğer tüm cihazlar **WORKER** modunda tek bir panel üzerinden yönetilir.
  * Toplam Farm Hashrate, anlık share'ler, en iyi zorluk (Best Diff) ve cihaz sıcaklıkları tek ekranda toplanır.
* 📶 **Çift WiFi & Otomatik Kesintisiz Yedekleme (Auto-Failover):**
  * 1. WiFi (Ev/İşyeri) kesilirse veya sinyal koparsa sistem otomatik olarak 2. Yedek WiFi ağına (örn. Mobil Hotspot) bağlanır.
  * İnternet kesilse bile madencilik durmaz.
* 🌡️ **Akıllı Aşırı Isınma Koruması (Thermal Throttling):**
  * Kart sıcaklığı panelden belirlenen sınır dereceyi (varsayılan: 75°C) aştığında madencilik kartı korumak için güvenli duraklatılır.
  * Kart soğuduğunda madencilik otomatik olarak kaldığı yerden devam eder.
* 🔄 **GitHub OTA & Kendi Kendini Güncelleme:**
  * Cihazlar doğrudan bu GitHub reposundaki `version.json` dosyasını sorgular.
  * Yeni sürüm çıktığında hem arayüzden tek tıkla hem de otomatik olarak cihazlar güncellenir.
* 💡 **Akıllı LED Durum Bildirimi:**
  * **Master Cihaz:** Her 2 saniyede bir kalp atışı gibi mavi LED nabzı atar.
  * **Worker Cihazlar:** Hashing durumunda hızlı pulse; "Bul (Identify)" tıklandığında flaşör moduna geçer.
* ⚡ **Zero-Touch Provisioning (ESP-NOW):**
  * Yeni eklenen kartlar tek bir butona basmadan Master'dan WiFi ve cüzdan ayarlarını kablosuz çeker.

---

## 🛠️ Desteklenen Kartlar & Varyantlar

| Model | Donanım | Açıklama | Binary |
|---|---|---|---|
| **ESP32-C3 SuperMini** | 0.96" I2C OLED | Ekranlı kompakt madenci | `nerdminer_esp32c3_oled.bin` |
| **ESP32-C3 SuperMini** | Ekransız (Mavi LED) | Güç tasarruflu farm madencisi | `nerdminer_esp32c3_headless.bin` |
| **ESP32-C3 SuperMini** | OLED + Dokunmatik | Buton kontrollü ekranlı model | `nerdminer_esp32c3_butonlu.bin` |
| **ESP32 DevKit v1** | 0.96" I2C OLED | Çift çekirdek (Dual-Core) ekranlı | `nerdminer_esp32_oled.bin` |
| **ESP32 DevKit v1** | Ekransız (Mavi LED) | Çift çekirdek yüksek hashrate farm | `nerdminer_esp32_headless.bin` |
| **ESP32 DevKit v1** | OLED + Fiziksel Buton | Çift çekirdek butonlu model | `nerdminer_esp32_butonlu.bin` |

---

## 📦 Kurulum & Hızlı Yükleme

1. **Seri Üretim / Hızlı USB Flasher:**
   * Klasördeki `SERI_URETIM_FLASHER.bat` dosyasını çalıştırın.
   * USB kablosunu taktığınız her karta (ESP32 veya ESP32-C3 otomatik algılanarak) saniyeler içinde yazılım yüklenir.
2. **Web Arayüzünden Güncelleme (OTA):**
   * Tarayıcınızdan `http://3dbrint.local/update` adresine girip `.bin` dosyasını seçerek kablosuz güncelleyebilirsiniz.

---

## 📜 Lisans & Yapımcı
* **Geliştirici:** 3dbrint & mefeoku
* **Temel Altyapı:** NerdMiner v2 Community Core
