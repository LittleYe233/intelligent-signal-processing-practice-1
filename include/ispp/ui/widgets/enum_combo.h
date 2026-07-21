#ifndef ISPP_UI_WIDGETS_ENUM_COMBO_H
#define ISPP_UI_WIDGETS_ENUM_COMBO_H

#include <imgui.h>

namespace ispp::ui {

/// @brief 强类型 enum ↔ ImGui::Combo 桥接。
template <typename Enum>
bool enumCombo(const char *label, Enum *value, const char *const *names,
               int count) {
    int current = static_cast<int>(*value);
    if (ImGui::Combo(label, &current, names, count)) {
        *value = static_cast<Enum>(current);
        return true;
    }
    return false;
}

} // namespace ispp::ui

#endif // ISPP_UI_WIDGETS_ENUM_COMBO_H
