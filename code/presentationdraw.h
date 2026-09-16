/*******************************************************************************
 *                                O P E N  T S
 ******************************************************************************/

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "rect.h"

class Surface;

struct PresentationRGB {
	std::uint8_t Red;
	std::uint8_t Green;
	std::uint8_t Blue;
};

enum class PresentationTextAlignment : std::uint8_t {
	LEFT,
	CENTER,
	RIGHT,
};

struct PresentationCreditLine {
	std::string Text;
	int SourceLine = 0;
	int Column = 0;
	PresentationTextAlignment Alignment = PresentationTextAlignment::LEFT;
	bool StartsGroup = false;
};

// Blends two RGB565 pixels with alpha in the inclusive 0..255 range.  The
// arithmetic intentionally follows the historical effect: alpha zero leaves
// source untouched and alpha 255 selects the overlay colour.
constexpr std::uint16_t Presentation_Blend_RGB565(std::uint16_t source, std::uint16_t overlay, std::uint8_t alpha)
{
	if (alpha == 0) return source;
	if (alpha == 255) return overlay;
	const unsigned source_alpha = 255U - alpha;
	const unsigned overlay_alpha = alpha;
	const std::uint16_t red = static_cast<std::uint16_t>(((((source & 0xF800U) * source_alpha) + ((overlay & 0xF800U) * overlay_alpha)) >> 8) & 0xF800U);
	const std::uint16_t green = static_cast<std::uint16_t>(((((source & 0x07E0U) * source_alpha) + ((overlay & 0x07E0U) * overlay_alpha)) >> 8) & 0x07E0U);
	const std::uint16_t blue = static_cast<std::uint16_t>(((((source & 0x001FU) * source_alpha) + ((overlay & 0x001FU) * overlay_alpha)) >> 8) & 0x001FU);
	return static_cast<std::uint16_t>(red | green | blue);
}

// Parses the public TSCREDIT text shape without accessing an owner-data file.
// Empty physical lines remain visible through StartsGroup on the next heading,
// while tabs retain their historical eight-column expansion.
inline std::vector<PresentationCreditLine> Presentation_Parse_Credits(std::string_view source)
{
	std::vector<PresentationCreditLine> lines;
	if (source.size() >= 3 && static_cast<unsigned char>(source[0]) == 0xEFU && static_cast<unsigned char>(source[1]) == 0xBBU && static_cast<unsigned char>(source[2]) == 0xBFU) {
		source.remove_prefix(3);
	}
	bool group_break = true;
	int source_line = 0;
	std::size_t offset = 0;
	while (offset <= source.size()) {
		std::size_t end = source.find_first_of("\r\n", offset);
		if (end == std::string_view::npos) end = source.size();
		std::string_view physical = source.substr(offset, end - offset);
		int column = 0;
		std::size_t first = 0;
		while (first < physical.size() && (physical[first] == ' ' || physical[first] == '\t')) {
			column = physical[first] == '\t' ? ((column + 8) & ~7) : column + 1;
			++first;
		}
		std::size_t last = physical.size();
		while (last > first && (physical[last - 1] == ' ' || physical[last - 1] == '\t')) --last;
		if (first != last) {
			PresentationCreditLine line;
			line.Text.assign(physical.substr(first, last - first));
			line.SourceLine = source_line;
			line.Column = column;
			line.Alignment = column < 3 ? PresentationTextAlignment::RIGHT : (column < 8 ? PresentationTextAlignment::CENTER : PresentationTextAlignment::LEFT);
			line.StartsGroup = group_break;
			lines.push_back(std::move(line));
			group_break = false;
		} else {
			group_break = true;
		}
		if (end == source.size()) break;
		if (source[end] == '\r' && end + 1 < source.size() && source[end + 1] == '\n') ++end;
		offset = end + 1;
		++source_line;
	}
	return lines;
}

// A narrow engine-surface replacement for the retained OwnerDraw remap-text
// call sites.  The font-name and spacing parameters preserve caller intent;
// this common entry point deliberately has no native drawing context.
int Presentation_Draw_Text_Remap(Surface & surface, char const * text, Rect const & rect, char const * font_name, PresentationRGB color, int flags, int char_spacing);
