/*******************************************************************************
 *                                O P E N  T S
 ******************************************************************************/

#include "always.h"

#include "presentationdraw.h"

#include "dialog.h"
#include "scheme.h"
#include "surface.h"

int Presentation_Draw_Text_Remap(Surface & surface, char const * text, Rect const & rect, char const *, PresentationRGB, int flags, int)
{
	if (text == nullptr || *text == '\0') return 0;
	TextPrintType style = TextPrintType(TPF_METAL12 | TPF_DROPSHADOW);
	if ((flags & 1) != 0) style = TextPrintType(style | TPF_CENTER);
	if ((flags & 2) != 0) style = TextPrintType(style | TPF_RIGHT);
	const Point2D point(rect.X, rect.Y);
	Fancy_Text_Print(text, surface, surface.Get_Rect(), point, Fetch_Scheme_By_Name("LightGold"), TBLACK, style);
	return Font_From_TPF(style)->String_Pixel_Width(text);
}
