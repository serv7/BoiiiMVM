#pragma once
#include <imgui.h>
#include <imgui_internal.h>

#include <string>

namespace ImGui
{
	enum ImGuiAligment_ { ImGuiAligment_Left, ImGuiAligment_Right, ImGuiAligment_Center };
	inline void TextAligned(const std::string& text, ImGuiAligment_ aligment = ImGuiAligment_Center)
	{
		const ImVec2& window_size = GetCurrentWindow()->Size;
		const ImVec2& text_size = CalcTextSize(text.c_str());

		if (aligment == ImGuiAligment_Left)
			SetCursorPosX(GetStyle().WindowPadding.x);
		if (aligment == ImGuiAligment_Right)
			SetCursorPosX(window_size.x - text_size.x - GetStyle().WindowPadding.x);
		if (aligment == ImGuiAligment_Center)
			SetCursorPosX((window_size.x - text_size.x) * 0.5f);

		Text(text.c_str());
	}
}