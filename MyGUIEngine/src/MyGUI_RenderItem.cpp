/*
 * This source file is part of MyGUI. For the latest info, see http://mygui.info/
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#include "MyGUI_Precompiled.h"
#include "MyGUI_RenderItem.h"
#include "MyGUI_LayerNode.h"
#include "MyGUI_LayerManager.h"
#include "MyGUI_Gui.h"
#include "MyGUI_RenderManager.h"
#include "MyGUI_DataManager.h"
#include "MyGUI_RenderManager.h"

namespace MyGUI
{

	RenderItem::RenderItem() :
		mGrouping(nullptr),
		mOutOfDate(false),
		mCurrentUpdate(true)
	{
	}

	RenderItem::~RenderItem()
	{
	}

	void RenderItem::renderToTarget(IRenderTarget* _target, bool _update)
	{
		if (mGrouping == nullptr)
			return;

		mCurrentUpdate = _update;
        
        for (VectorDrawItem::iterator iter = mDrawItems.begin(); iter != mDrawItems.end(); ++iter)
        {
            
            (*iter)->doRender(_target);
            
        }
        
        mOutOfDate = false;

	}

	void RenderItem::removeDrawItem(ISubWidget* _item)
	{
		for (VectorDrawItem::iterator iter = mDrawItems.begin(); iter != mDrawItems.end(); ++iter)
		{
			if ((*iter) == _item)
			{
				mDrawItems.erase(iter);
				mOutOfDate = true;

			
				// если все отдетачились, расскажем отцу
				if (mDrawItems.empty())
				{
					mGrouping = nullptr;
				}

				return;
			}
		}
		MYGUI_EXCEPT("DrawItem not found");
	}

	void RenderItem::addDrawItem(ISubWidget* _item)
	{

// проверяем только в дебаге
#if MYGUI_DEBUG_MODE == 1
		for (VectorDrawItem::iterator iter = mDrawItems.begin(); iter != mDrawItems.end(); ++iter)
		{
			MYGUI_ASSERT((*iter) != _item, "DrawItem exist");
		}
#endif

		mDrawItems.push_back(_item);
		mOutOfDate = true;
	}

	

	void RenderItem::setGrouping(const void* _grouping)
	{
		if (mGrouping == _grouping)
			return;
        mGrouping = _grouping;
    }

	const void* RenderItem::getGrouping()
	{
		return mGrouping;
	}


	void RenderItem::outOfDate()
	{
		mOutOfDate = true;
	}

	bool RenderItem::isOutOfDate() const
	{
		return mOutOfDate;
	}


	bool RenderItem::getCurrentUpdate() const
	{
		return mCurrentUpdate;
	}

	
} // namespace MyGUI
