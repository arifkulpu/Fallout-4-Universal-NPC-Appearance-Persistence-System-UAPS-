# Fallout 4 Universal NPC Appearance & Persistence System (UAPS)

[Türkçe açıklama için aşağıya bakın / See below for Turkish description]

A powerful F4SE plugin designed to make NPC appearance changes permanent and persistent across saves, cell changes, and game sessions. Now compatible with both **Standard (1.10.163)** and **Next-Gen / AE (1.10.980+)** versions.

## Features & Architecture (nlohmann/json)
- **JSON-Based Storage**: NPC data is stored asynchronously in `Data/F4SE/Plugins/UAPS_Data.json` to prevent FPS drops during saves.
- **FormID String Format**: FormIDs are saved securely as `"ModName|LocalID"` (e.g., `"Fallout4.esm|00002F1F"`) to track Load Order shifts and avoid form ID drift.
- **3 Persistence Layers**:
  1. **Geometry (FaceGen)**: Face bone structure, morphs, and body slider (CBBE) values.
  2. **Assets (HeadParts)**: Hair, eyes, and eyebrows. Employs `LookupFormByID` to safely handle uninstalled mods without crashing.
  3. **Tints (Skin/Makeup)**: Skin color, makeup, and tattoos. Injected right before `UpdateAppearance()` to prevent "Brown Face" bugs.
- **Smart Tracking Logic**:
  - **Unique NPCs**: Tracked by their `BaseID` (so changes persist across new games!).
  - **Settlers**: Tracked by their `RefID` (only the specific NPC changes).
  - **Raiders/Gunners**: (Optional) Temporary RefID tracking.
- **On3DLoad Hook**: Guarantees your changes are the final applied appearance by applying them exactly when the NPC is rendered (`Actor::Load3D`).

## Usage
1. Open the console and click on an NPC.
2. Type `slm` to open the LooksMenu.
3. Modify the appearance as you wish and close the menu.
4. The appearance is asynchronously saved to JSON and will persist forever!

---

# Fallout 4 Evrensel NPC Görünüm ve Kalıcılık Sistemi (UAPS)

NPC görünüm değişikliklerini kayıtlar (save), hücre değişimleri ve oyun oturumları arasında kalıcı hale getiren güçlü bir F4SE eklentisidir. Artık hem **Standart (1.10.163)** hem de **Next-Gen / AE (1.10.980+)** sürümleriyle uyumludur.

## Mimari ve Özellikler (nlohmann/json Entegrasyonu)
- **Asenkron JSON Kaydı**: Kayıt dosyası şişmesini (save bloat) engellemek ve FPS düşüşü yaşatmamak için veriler arka planda asenkron olarak `Data/F4SE/Plugins/UAPS_Data.json` dosyasına yazılır. Sadece oyuncunun dokunduğu (düzenlediği) NPC'ler kaydedilir.
- **Dinamik FormID Takibi**: FormID'ler Hex formatında `Modİsmi|LokalID` string'i olarak saklanır (`Fallout4.esm|00002F1F` gibi). Bu, Load Order değiştiğinde kaymaların (form shift) önüne geçer.
- **3 Temel Kalıcılık Katmanı**:
  1. **Geometry (FaceGen)**: Yüz kemik yapısı ve vücut (CBBE/BodySlide) değerleri.
  2. **Assets (HeadParts)**: Saç, kaş ve göz modelleri. Eksik/silinmiş modlar için `LookupFormByID` kontrolü ile oyunun çökmesi engellenir.
  3. **Tints (Skin/Makeup)**: Ten rengi, makyaj ve dövme katmanları. "Brown Face" (kahverengi yüz) hatasını engellemek için doğrudan uygulanır.
- **Kayıt Mantığı**:
  - **Unique (Eşsiz)**: BaseID ile kaydedilir. Yeni bir oyuna başlasanız bile Piper aynı kalır!
  - **Settler (Yerleşimci)**: RefID ile kaydedilir. Sadece o spesifik kişi değişir.
  - **Raider/Gunner**: Sadece opsiyonel olarak geçici RefID ile tutulur.
- **Teknik İş Akışı**: Oyun motorunun `Actor::Load3D` fonksiyonu hook'lanmıştır. Böylece RobCo Patcher vb. modlarla çakışma yaşanmaz, UAPS "son sözü" söyler.

## Kullanım
1. Konsolu açın ve bir NPC'ye tıklayın.
2. LooksMenu'yü açmak için `slm` yazın.
3. Görünümü dilediğiniz gibi değiştirin ve menüyü kapatın.
4. Görünümünüz JSON sistemine kaydedilmiştir ve sonsuza dek kalıcı olacaktır!

## Gereksinimler
- [Fallout 4](https://store.steampowered.com/app/377160/Fallout_4/) (Sürüm 1.10.163 veya Next-Gen/AE 1.10.980+)
- [F4SE](https://f4se.silverlock.org/) (Fallout 4 Script Extender)
- [Address Library for F4SE Plugins](https://www.nexusmods.com/fallout4/mods/47327)
