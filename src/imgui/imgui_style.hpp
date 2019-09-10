#pragma once

#include <imgui.h>
#include <imgui_internal.h>

namespace ImGui
{

	IMGUI_API void StyleColorsDarkCodz1(ImGuiStyle* dst = NULL); // https://github.com/codz01
	IMGUI_API void StyleColorsCherry(ImGuiStyle* dst = NULL); // https://github.com/ocornut/imgui/issues/707
	IMGUI_API void StyleColorsLightGreen(ImGuiStyle* dst = NULL); // https://github.com/ocornut/imgui/pull/1776
	IMGUI_API void StyleColorsUE(ImGuiStyle* dst = NULL); // https://github.com/ocornut/imgui/issues/707
	IMGUI_API void StyleCorporateGrey(ImGuiStyle* dst = NULL); // https://github.com/ocornut/imgui/issues/707

	inline void ToggleButton(const char* str_id, bool* v)
	{
		Text(str_id); ImGui::SameLine();

		ImVec2 p = ImGui::GetCursorScreenPos();
		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		float height_modifier = 0.8;
		float width_modifier = 2.0f;
		float height = ImGui::GetFrameHeight() * height_modifier;
		float width = height * width_modifier;
		float radius = height * 0.50f;

		ImGui::InvisibleButton(str_id, ImVec2(width, height));
		if (ImGui::IsItemClicked())
			*v = !*v;

		float t = *v ? 1.0f : 0.0f;

		ImGuiContext& g = *GImGui;
		float ANIM_SPEED = 0.08f;
		if (g.LastActiveId == g.CurrentWindow->GetID(str_id))// && g.LastActiveIdTimer < ANIM_SPEED)
		{
			float t_anim = ImSaturate(g.LastActiveIdTimer / ANIM_SPEED);
			t = *v ? (t_anim) : (1.0f - t_anim);
		}

		ImU32 col_bg;
		if (ImGui::IsItemHovered())
			col_bg = ImGui::GetColorU32(ImLerp(ImVec4(0.65f, 0.65f, 0.65f, 1.0f), ImVec4(0.64f, 0.83f, 0.34f, 1.0f), t));
		else
			col_bg = ImGui::GetColorU32(ImLerp(ImVec4(0.75f, 0.75f, 0.75f, 1.0f), ImVec4(0.56f, 0.83f, 0.26f, 1.0f), t));

		draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), col_bg, height * 0.5f);
		draw_list->AddCircleFilled(ImVec2(p.x + radius + t * (width - radius * 2.0f), p.y + radius), radius - 1.5f, IM_COL32(255, 255, 255, 255));
	}

} /* ImGui */
