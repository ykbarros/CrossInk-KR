#pragma once

#include <GfxRenderer.h>
#include <I18n.h>

#include "CrossPointSettings.h"
#include "activities/Activity.h"
#include "util/ButtonNavigator.h"

class MappedInputManager;

// Lets the user set per-language default reader fonts.
// Each row corresponds to one UI language; tapping opens the standard
// font picker and saves the result to that language's slot in
// SETTINGS.languageFonts (via saveActiveReaderFontForLanguage).
class LanguageFontsActivity final : public Activity {
 public:
  explicit LanguageFontsActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("LanguageFonts", renderer, mappedInput) {}

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  void openFontPickerForSelected();
  std::string currentFontLabelForLanguage(Language lang) const;

  ButtonNavigator buttonNavigator_;
  int selectedIndex_ = 0;

  // SETTINGS snapshot held while the font picker is open
  uint8_t savedFontFamily_ = 0;
  uint8_t savedFontSize_ = 0;
  char savedSdFontFamilyName_[64] = "";

  static constexpr uint8_t totalItems_ = getLanguageCount();
};
