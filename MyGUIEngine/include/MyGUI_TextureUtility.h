/*
 * This source file is part of MyGUI. For the latest info, see http://mygui.info/
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#ifndef MYGUI_TEXTURE_UTILITY_H_
#define MYGUI_TEXTURE_UTILITY_H_

#include "MyGUI_Prerequest.h"
#include "MyGUI_Colour.h"
#include "MyGUI_RenderFormat.h"

namespace MyGUI
{

	namespace texture_utility
	{

		MYGUI_EXPORT const IntSize& getTextureSize(const std::string& _texture, bool _cache = true);
		MYGUI_EXPORT uint32 toColourARGB(const Colour& _colour);


	} // namespace texture_utility

} // namespace MyGUI

#endif // MYGUI_TEXTURE_UTILITY_H_
