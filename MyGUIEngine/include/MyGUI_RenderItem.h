/*
 * This source file is part of MyGUI. For the latest info, see http://mygui.info/
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#ifndef MYGUI_RENDER_ITEM_H_
#define MYGUI_RENDER_ITEM_H_

#include "MyGUI_Prerequest.h"
#include "MyGUI_ISubWidget.h"
#include "MyGUI_VertexData.h"
#include "MyGUI_IRenderTarget.h"

namespace MyGUI
{

	typedef std::vector<ISubWidget*> VectorDrawItem;

	class MYGUI_EXPORT RenderItem
	{
	public:
		RenderItem();
		virtual ~RenderItem();

		void renderToTarget(IRenderTarget* _target, bool _update);

		void setGrouping(const void* _value);
		const void* getGrouping();


		void addDrawItem(ISubWidget* _item);
		void removeDrawItem(ISubWidget* _item);
		
		void outOfDate();
		bool isOutOfDate() const;

		bool getCurrentUpdate() const;
		
	private:
#if MYGUI_DEBUG_MODE == 1
		std::string mTextureName;
#endif

		const void* mGrouping;

		bool mOutOfDate;
		VectorDrawItem mDrawItems;

		bool mCurrentUpdate;
		
	};

} // namespace MyGUI

#endif // MYGUI_RENDER_ITEM_H_
