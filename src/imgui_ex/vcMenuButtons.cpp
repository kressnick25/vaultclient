#include "vcMenuButtons.h"
#include "gl/vcTexture.h"

#include "imgui.h"

bool vcMenuBarButton(vcTexture *pUITexture, const char *pButtonName, const char *pKeyCode, const vcMenuBarButtonIcon buttonIndex, vcMenuBarButtonGap gap, bool selected /*= false*/)
{
  const float buttonSize = 24.f;
  const float textureRelativeButtonSize = 256.f;
  const float buttonUVSize = buttonSize / textureRelativeButtonSize;
  const ImVec4 DefaultBGColor = ImVec4(0, 0, 0, 0);
  const ImVec4 EnabledColor = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);

  const float MBtn_Gap = 10.f;
  const float MBtn_Padding = 2.f;

  float buttonX = (buttonIndex % (int)(textureRelativeButtonSize / buttonSize)) * buttonUVSize;
  float buttonY = (buttonIndex / (int)(textureRelativeButtonSize / buttonSize)) * buttonUVSize;

  if (gap == vcMBBG_SameGroup)
    ImGui::SameLine(0.f, MBtn_Padding);
  else if (gap == vcMBBG_NewGroup)
    ImGui::SameLine(0.f, MBtn_Gap);

  ImGui::PushID(pButtonName);
  bool retVal = ImGui::ImageButton(pUITexture, ImVec2(buttonSize, buttonSize), ImVec2(buttonX, buttonY), ImVec2(buttonX + buttonUVSize, buttonY + buttonUVSize), 2, selected ? EnabledColor : DefaultBGColor);
  if (ImGui::IsItemHovered())
  {
    if (pKeyCode == nullptr)
      ImGui::SetTooltip("%s", pButtonName);
    else
      ImGui::SetTooltip("%s [%s]", pButtonName, pKeyCode);
  }
  ImGui::PopID();

  return retVal;
}