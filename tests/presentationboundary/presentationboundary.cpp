#include "presentationdraw.h"

#include <cassert>
#include <string_view>

int main()
{
	// The header is deliberately self-contained and contains no native-window
	// surface. These checks keep that contract visible in a no-owner-data test.
	static_assert(Presentation_Blend_RGB565(0x1234, 0xFEDC, 0) == 0x1234);
	static_assert(Presentation_Blend_RGB565(0x1234, 0xFEDC, 255) == 0xFEDC);
	static_assert(Presentation_Blend_RGB565(0xF800, 0x001F, 128) == 0x780F);

	const auto lines = Presentation_Parse_Credits("\xEF\xBB\xBFLead\r\n\r\n\tTitle\n  Left\n        Right\n");
	assert(lines.size() == 4);
	assert(lines[0].Text == "Lead");
	assert(lines[0].SourceLine == 0);
	assert(lines[0].Alignment == PresentationTextAlignment::RIGHT);
	assert(lines[0].StartsGroup);
	assert(lines[1].Text == "Title");
	assert(lines[1].Column == 8);
	assert(lines[1].Alignment == PresentationTextAlignment::LEFT);
	assert(lines[1].StartsGroup);
	assert(lines[2].Text == "Left");
	assert(lines[2].Alignment == PresentationTextAlignment::RIGHT);
	assert(lines[3].Text == "Right");
	assert(lines[3].Alignment == PresentationTextAlignment::LEFT);
	assert(Presentation_Parse_Credits(" \t\r\n").empty());

	constexpr std::string_view header_contract = "PresentationRGB PresentationTextAlignment PresentationCreditLine";
	assert(header_contract.find("HWND") == std::string_view::npos);
	return 0;
}
