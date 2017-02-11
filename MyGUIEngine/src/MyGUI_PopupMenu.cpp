/*
 * This source file is part of MyGUI. For the latest info, see http://mygui.info/
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#include "MyGUI_Precompiled.h"
#include "MyGUI_PopupMenu.h"

namespace MyGUI
{
    MYGUI_IMPL_TYPE_NAME(PopupMenu)

	PopupMenu::PopupMenu()
	{
		mHideByLostKey = true;
	}

} // namespace MyGUI
