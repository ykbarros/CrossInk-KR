#include "LanguageFontsActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>

#include <algorithm>
#include <cstring>
#include <iterator>

#include "CrossPointSettings.h"
#include "FontSelectionActivity.h"
#include "I18nKeys.h"
#include "MappedInputManager.h"
#include "SdCardFontSystem.h"
#include "components/UITheme.h"
#include "fontIds.h"

namespace {
const CrossPointSettings::LanguageFontSetting* findSlotForCode(const char* code) {
  for (const auto& slot : SETTINGS.languageFonts) {
    if (strcmp(slot.languageCode, code) == 0) return &slot;
  }
  return nullptr;
}

// Lowercased copy of a 2-3 letter language code (matches normalizeLanguageCode behaviour).
void lowerCopy(const char* in, char* out, size_t outSize) {
  size_t i = 0;
  for (; in[i] != '\0' && i + 1 < outSize; i++) {
    char c = in[i];
    if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    out[i] = c;
  }
  out[i] = '\0';
}

std::string builtinFontName(uint8_t family) {
  switch (family) {
    case CrossPointSettings::BITTER:
      return I18N.get(StrId::STR_BITTER);
    case CrossPointSettings::CHAREINK:
      return I18N.get(StrId::STR_CHAREINK);
    case CrossPointSettings::LEXENDDECA:
    default:
      return I18N.get(StrId::STR_LEXEND_DECA);
  }
}
}  // namespace

void LanguageFontsActivity::onEnter() {
  Activity::onEnter();
  sdFontSystem.refreshIfDirty();
  selectedIndex_ = 0;
  requestUpdate();
}

void LanguageFontsActivity::onExit() { Activity::onExit(); }

std::string LanguageFontsActivity::currentFontLabelForLanguage(Language lang) const {
  char code[CrossPointSettings::LANGUAGE_FONT_CODE_LEN];
  lowerCopy(LANGUAGE_CODES[static_cast<int>(lang)], code, sizeof(code));

  const auto* slot = findSlotForCode(code);
  if (!slot) return "—";

  if (slot->sdFontFamilyName[0] != '\0') return slot->sdFontFamilyName;
  return builtinFontName(slot->fontFamily);
}

void LanguageFontsActivity::openFontPickerForSelected() {
  const Language lang = static_cast<Language>(SORTED_LANGUAGE_INDICES[selectedIndex_]);

  // Snapshot current SETTINGS so we can restore them after the picker.
  savedFontFamily_ = SETTINGS.fontFamily;
  savedFontSize_ = SETTINGS.fontSize;
  strncpy(savedSdFontFamilyName_, SETTINGS.sdFontFamilyName, sizeof(savedSdFontFamilyName_) - 1);
  savedSdFontFamilyName_[sizeof(savedSdFontFamilyName_) - 1] = '\0';

  // Preload SETTINGS with the slot's current value (or leave existing SETTINGS as the
  // initial guess if no slot exists yet) so FontSelectionActivity highlights the right row.
  char code[CrossPointSettings::LANGUAGE_FONT_CODE_LEN];
  lowerCopy(LANGUAGE_CODES[static_cast<int>(lang)], code, sizeof(code));
  if (const auto* slot = findSlotForCode(code)) {
    SETTINGS.fontFamily = slot->fontFamily;
    SETTINGS.fontSize = slot->fontSize;
    strncpy(SETTINGS.sdFontFamilyName, slot->sdFontFamilyName, sizeof(SETTINGS.sdFontFamilyName) - 1);
    SETTINGS.sdFontFamilyName[sizeof(SETTINGS.sdFontFamilyName) - 1] = '\0';
  }

  startActivityForResult(
      std::make_unique<FontSelectionActivity>(renderer, mappedInput, &sdFontSystem.registry()),
      [this, lang](const ActivityResult&) {
        const bool changed = SETTINGS.fontFamily != savedFontFamily_ || SETTINGS.fontSize != savedFontSize_ ||
                             strcmp(SETTINGS.sdFontFamilyName, savedSdFontFamilyName_) != 0;

        if (changed) {
          char code[CrossPointSettings::LANGUAGE_FONT_CODE_LEN];
          lowerCopy(LANGUAGE_CODES[static_cast<int>(lang)], code, sizeof(code));
          SETTINGS.saveActiveReaderFontForLanguage(code);
        }

        // Restore the active SETTINGS so this menu doesn't change the global reader font.
        SETTINGS.fontFamily = savedFontFamily_;
        SETTINGS.fontSize = savedFontSize_;
        strncpy(SETTINGS.sdFontFamilyName, savedSdFontFamilyName_, sizeof(SETTINGS.sdFontFamilyName) - 1);
        SETTINGS.sdFontFamilyName[sizeof(SETTINGS.sdFontFamilyName) - 1] = '\0';

        if (changed) SETTINGS.saveToFile();
        requestUpdate();
      });
}

void LanguageFontsActivity::loop() {
  if (mappedInput.wasPressed(MappedInputManager::Button::Back)) {
    finish();
    return;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    openFontPickerForSelected();
    return;
  }

  const int pageItems = UITheme::getNumberOfItemsPerPage(renderer, true, false, true, false);

  buttonNavigator_.onNextRelease([this] {
    selectedIndex_ = ButtonNavigator::nextIndex(selectedIndex_, totalItems_);
    requestUpdate();
  });
  buttonNavigator_.onPreviousRelease([this] {
    selectedIndex_ = ButtonNavigator::previousIndex(selectedIndex_, totalItems_);
    requestUpdate();
  });
  buttonNavigator_.onNextContinuous([this, pageItems] {
    selectedIndex_ = ButtonNavigator::nextPageIndex(selectedIndex_, totalItems_, pageItems);
    requestUpdate();
  });
  buttonNavigator_.onPreviousContinuous([this, pageItems] {
    selectedIndex_ = ButtonNavigator::previousPageIndex(selectedIndex_, totalItems_, pageItems);
    requestUpdate();
  });
}

void LanguageFontsActivity::render(RenderLock&&) {
  renderer.clearScreen();

  const auto pageWidth = renderer.getScreenWidth();
  const auto pageHeight = renderer.getScreenHeight();
  const auto& metrics = UITheme::getInstance().getMetrics();

  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, tr(STR_LANGUAGE_FONTS));

  const int contentTop = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing;
  const int contentHeight = pageHeight - contentTop - metrics.buttonHintsHeight - metrics.verticalSpacing;

  GUI.drawList(
      renderer, Rect{0, contentTop, pageWidth, contentHeight}, totalItems_, selectedIndex_,
      [](int index) { return I18N.getLanguageName(static_cast<Language>(SORTED_LANGUAGE_INDICES[index])); }, nullptr,
      nullptr,
      [this](int index) {
        return currentFontLabelForLanguage(static_cast<Language>(SORTED_LANGUAGE_INDICES[index]));
      },
      true);

  const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer();
}
