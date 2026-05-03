# Fallout 4 Universal NPC Appearance & Persistence System (UAPS)

[Türkçe açıklama için aşağıya bakın / See below for Turkish description]

A powerful F4SE plugin designed to make NPC appearance changes permanent and persistent across saves, cell changes, and game sessions. Now fully compatible with **Standard (1.10.163)**, **Next-Gen 1 (1.10.980-984)**, and **Next-Gen 2 (1.11.191+)** versions.

## Features & Architecture
- **Dual-Layer Storage**: 
  - **Global JSON**: Unique NPCs are stored in `UAPS_Data.json` (linked by ModName|FormID) to persist across different save files.
  - **F4SE Co-save (Serialization)**: Non-unique NPCs (Settlers/Raiders) are stored directly inside your save file using F4SE Serialization to ensure persistence even with dynamic IDs.
- **Full Tint Persistence**: Makeup, face paint, tattoos, and skin details are now 100% persistent. Injected using a clean reconstruction method to prevent "Brown Face" bugs and conflicts with mods like **Botox**.
- **Appearance Source Tracking**: Automatically detects if an NPC is using a template (faceNPC) and maps the appearance source correctly.
- **FormID String Format**: Global data is saved as `"ModName|LocalID"` to handle Load Order changes.
- **On3DLoad Hook**: Guarantees your changes are the final applied appearance by applying them exactly when the NPC is rendered (`Actor::Load3D`).

## Usage
1. Open the console and click on an NPC.
2. Type `slm` to open the LooksMenu.
3. Modify the appearance as you wish and close the menu.
4. The appearance is saved and will persist forever!

---

# Fallout 4 Evrensel NPC Görünüm ve Kalıcılık Sistemi (UAPS)

NPC görünüm değişikliklerini kayıtlar (save), hücre değişimleri ve oyun oturumları arasında kalıcı hale getiren güçlü bir F4SE eklentisidir. Artık **Standart (1.10.163)**, **Next-Gen 1 (1.10.980-984)** ve **Next-Gen 2 (1.11.191+)** sürümleriyle tam uyumludur.

## Mimari ve Özellikler
- **Çift Katmanlı Kayıt Sistemi**: 
  - **Global JSON**: Eşsiz (Unique) NPC'ler `UAPS_Data.json` dosyasına (Modİsmi|FormID) kaydedilir. Bu sayede farklı save dosyalarında bile Piper aynı kalır.
  - **F4SE Co-save (Serialization)**: Yerleşimciler (Settlers) ve akıncılar gibi dinamik ID'li NPC'ler doğrudan save dosyanızın (`.f4se`) içine kaydedilir. Böylece ID'leri değişse bile kalıcılık korunur.
- **Tam Tint (Makyaj) Kalıcılığı**: Makyaj, yüz boyası, dövme ve ten detayları artık %100 kalıcıdır. "Brown Face" hatasını ve **Botox** gibi modlarla olan çakışmaları önlemek için temiz bir yeniden oluşturma yöntemi kullanılır.
- **Görünüş Kaynağı (Template) Takibi**: NPC'nin görünüşünü bir şablondan (faceNPC) alıp almadığını otomatik tespit eder ve kaynağı doğru eşleştirir.
- **Dinamik FormID Takibi**: Global veriler `Modİsmi|LokalID` string'i olarak saklanır, böylece mod sıralamanız (Load Order) değişse bile bozulma yaşanmaz.
- **Teknik İş Akışı**: Oyun motorunun `Actor::Load3D` fonksiyonu hook'lanmıştır. UAPS "son sözü" söyler ve diğer modlarla çakışmaz.

## Kullanım
1. Konsolu açın ve bir NPC'ye tıklayın.
2. LooksMenu'yü açmak için `slm` yazın.
3. Görünümü dilediğiniz gibi değiştirin ve menüyü kapatın.
4. Görünümünüz sisteme kaydedilmiştir ve sonsuza dek kalıcı olacaktır!

## Gereksinimler
- [Fallout 4](https://store.steampowered.com/app/377160/Fallout_4/) (v1.10.163 / v1.10.980 / v1.10.984 / v1.11.191+)
- [F4SE](https://f4se.silverlock.org/) (Sürüm 0.6.23 / 0.7.2 / 0.7.7+)
- [Address Library for F4SE Plugins](https://www.nexusmods.com/fallout4/mods/47327) (Oyun sürümünüzle uyumlu olan .bin dosyası gereklidir)

## License / Lisans

Copyright (c) 2026 Arif KULPU. All Rights Reserved. — Tüm Hakları Saklıdır.
See `LICENSE.md` for details.

Unauthorized copying, modification, distribution, or use of this software
and associated documentation files, in any medium, is strictly prohibited
without the express prior written permission of the copyright holder.

This software is provided "as is", without warranty of any kind.
The copyright holder shall not be liable for any claim, damages, or other
liability arising from the use of this software.

---

Telif Hakkı (c) 2026 Arif KULPU. Tüm Hakları Saklıdır.
Detaylar için `LICENSE.md` dosyasına bakınız.

Bu yazılımın ve ilgili belgelerinin herhangi bir ortamda izinsiz olarak
kopyalanması, değiştirilmesi, dağıtılması veya kullanılması, telif hakkı
sahibinin açık ve önceden yazılı izni olmaksızın kesinlikle yasaktır.

Bu yazılım "olduğu gibi" sunulmaktadır; herhangi bir garanti verilmemektedir.
Telif hakkı sahibi, bu yazılımın kullanımından doğan hiçbir talep, zarar
veya yükümlülükten sorumlu tutulamaz.
