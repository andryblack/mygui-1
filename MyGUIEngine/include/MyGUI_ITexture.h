/*
 * This source file is part of MyGUI. For the latest info, see http://mygui.info/
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#ifndef MYGUI_I_TEXTURE_H_
#define MYGUI_I_TEXTURE_H_

#include "MyGUI_Prerequest.h"
#include "MyGUI_IObject.h"
#include <string>

namespace MyGUI
{
	class ITexture;

    class MYGUI_EXPORT ITexture :
        public IObject
	{
        MYGUI_RTTI_DERIVED( ITexture )
	public:
		virtual ~ITexture() { }

		virtual const std::string& getName() const = 0;

		virtual void destroy() = 0;

		virtual int getWidth() = 0;
		virtual int getHeight() = 0;
	};

} // namespace MyGUI

#endif // MYGUI_I_TEXTURE_H_
