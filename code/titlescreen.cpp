/*******************************************************************************
 *                                O P E N  T S
 ******************************************************************************/

#include "always.h"

#include "titlescreen.h"

#include "ccfile.h"
#include "convert.h"
#include "draw.h"
#include "palette.h"
#include "pcx.h"
#include "surface.h"

void Load_Title_Screen(char const * name, Surface * surface, PaletteClass * palette)
{
	if (name == nullptr || surface == nullptr) return;
	CCFileClass file(name);
	Surface *load_buffer = Read_PCX_File(file, palette);
	if (load_buffer == nullptr) return;
	const int x = (surface->Get_Width() - load_buffer->Get_Width()) / 2;
	const int y = (surface->Get_Height() - load_buffer->Get_Height()) / 2;
	if (palette != nullptr && load_buffer->Bytes_Per_Pixel() == 1) {
		ConvertClass drawer(*palette, *palette, *surface);
		Blit_Block(*surface, drawer, *load_buffer, load_buffer->Get_Rect(), Point2D(x, y), surface->Get_Rect());
	} else {
		surface->Blit_From(surface->Get_Rect(), Rect(x, y, load_buffer->Get_Width(), load_buffer->Get_Height()), *load_buffer, load_buffer->Get_Rect(), load_buffer->Get_Rect());
	}
	delete load_buffer;
}
