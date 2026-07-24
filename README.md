> **This is a personal fork of [CrossInk](https://github.com/uxjulia/CrossInk)** (itself a fork of [CrossPoint Reader](https://github.com/crosspoint-reader/crosspoint-reader)) adding Korean (한국어) support and SD-card-only font management.

## What I added on top of CrossInk

- **Korean (한국어) UI language** with full translations for the device interface.
- **Hangul-capable EPUB reading** via SD-card fonts — install a Korean font like [Pretendard](https://github.com/orioncactus/pretendard) as a `.cpfont` on your SD card and select it as your reader font to read Korean books cleanly.
- **Removed the on-device font downloader.** Fonts are installed by copying `.cpfont` files directly to the SD card, which is simpler and avoids needing WiFi just to add a typeface.
