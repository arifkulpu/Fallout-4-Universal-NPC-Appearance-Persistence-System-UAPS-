# Fallout 4 Universal NPC Appearance & Persistence System (UAPS)

[Türkçe açıklama için aşağıya bakın / See below for Turkish description]

A powerful F4SE plugin designed to make NPC appearance changes permanent and persistent across saves, cell changes, and game sessions.

## Features
- **Appearance Persistence**: Automatically saves morphs, headparts, and tint layers (makeup, skin color, etc.) when you modify an NPC via `ShowLooksMenu` (SLM).
- **Automatic NPC Detection**: When you close the LooksMenu, the plugin automatically detects if you edited the Player or a selected NPC (via console) and saves their appearance instantly.
- **Dynamic Application**: Re-applies the saved appearance whenever the NPC is loaded into a cell, preventing the game from resetting them to their base template.
- **JSON-Based Storage**: NPC data is stored in easy-to-read JSON files under `Data/F4SE/Plugins/UniversalAppearance/NPCs/`.
- **F4SE Native Functions**: Includes native functions for Papyrus integration to trigger saves, reloads, or removal manually (`OnSLMMenuExit`, `ReloadWatchlist`, `RemoveFromWatchlist`).
- **Persistence Management**: Automatically sets tracked NPCs as persistent to prevent the engine's garbage collector from deleting them if they are far from the player.

## Usage
1. Open the console and click on an NPC (their RefID will appear).
2. Type `slm` to open the LooksMenu.
3. Modify the appearance as you wish and close the menu.
4. The appearance is now automatically saved and will persist forever!

## Requirements
- [Fallout 4](https://store.steampowered.com/app/377160/Fallout_4/) (Version 1.10.163 or compatible)
- [F4SE](https://f4se.silverlock.org/) (Fallout 4 Script Extender)
- [Address Library for F4SE Plugins](https://www.nexusmods.com/fallout4/mods/47327)

## Installation
1. Ensure F4SE is installed correctly.
2. Copy `UniversalAppearance.dll` to your `Data/F4SE/Plugins/` directory.
3. (Optional) If you have the Papyrus script, place it in `Data/Scripts/`.

---

# Fallout 4 Evrensel NPC Görünüm ve Kalıcılık Sistemi (UAPS)

NPC görünüm değişikliklerini kayıtlar (save), hücre değişimleri ve oyun oturumları arasında kalıcı hale getiren güçlü bir F4SE eklentisidir.

## Özellikler
- **Görünüm Kalıcılığı**: `ShowLooksMenu` (SLM) aracılığıyla bir NPC'yi değiştirdiğinizde morph'ları, kafa parçalarını (headparts) ve katmanları (tints - makyaj, deri rengi vb.) otomatik olarak kaydeder.
- **Otomatik NPC Algılama**: LooksMenu kapatıldığında, eklenti oyuncuyu mu yoksa (konsol üzerinden seçilmiş) bir NPC'yi mi düzenlediğinizi otomatik olarak algılar ve görünümlerini anında kaydeder.
- **Dinamik Uygulama**: NPC bir hücreye yüklendiğinde kayıtlı görünümü otomatik olarak yeniden uygular, böylece oyunun NPC'yi varsayılan şablonuna sıfırlamasını engeller.
- **JSON Tabanlı Depolama**: NPC verileri `Data/F4SE/Plugins/UniversalAppearance/NPCs/` altında okunabilir JSON dosyalarında saklanır.
- **F4SE Native Fonksiyonlar**: Manuel kayıt, yükleme veya listeden çıkarma tetiklemek için Papyrus entegrasyonu sunar (`OnSLMMenuExit`, `ReloadWatchlist`, `RemoveFromWatchlist`).
- **Kalıcılık Yönetimi**: Takip edilen NPC'leri otomatik olarak "persistent" (kalıcı) olarak işaretler, böylece oyunun temizlik mekanizması (GC) tarafından silinmelerini önler.

## Kullanım
1. Konsolu açın ve bir NPC'ye tıklayın (RefID'si ekranda görünecektir).
2. LooksMenu'yü açmak için `slm` yazın.
3. Görünümü dilediğiniz gibi değiştirin ve menüyü kapatın.
4. Görünüm artık otomatik olarak kaydedilmiştir ve sonsuza dek kalıcı olacaktır!

## Gereksinimler
- [Fallout 4](https://store.steampowered.com/app/377160/Fallout_4/) (Sürüm 1.10.163 veya uyumlu)
- [F4SE](https://f4se.silverlock.org/) (Fallout 4 Script Extender)
- [Address Library for F4SE Plugins](https://www.nexusmods.com/fallout4/mods/47327)

## Kurulum
1. F4SE'nin doğru kurulduğundan emin olun.
2. `UniversalAppearance.dll` dosyasını `Data/F4SE/Plugins/` dizinine kopyalayın.
3. (Opsiyonel) Eğer Papyrus scripti varsa `Data/Scripts/` klasörüne yerleştirin.
