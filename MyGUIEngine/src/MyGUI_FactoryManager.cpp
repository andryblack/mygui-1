/*
 * This source file is part of MyGUI. For the latest info, see http://mygui.info/
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#include "MyGUI_Precompiled.h"
#include "MyGUI_FactoryManager.h"

namespace MyGUI
{

	template <> FactoryManager* Singleton<FactoryManager>::msInstance = nullptr;
	template <> const char* Singleton<FactoryManager>::mClassTypeName = "FactoryManager";

	FactoryManager::FactoryManager() :
		mIsInitialise(false)
	{
	}

	void FactoryManager::initialise()
	{
		MYGUI_ASSERT(!mIsInitialise, getClassTypeName() << " initialised twice");
		MYGUI_LOG(Info, "* Initialise: " << getClassTypeName());

		MYGUI_LOG(Info, getClassTypeName() << " successfully initialized");
		mIsInitialise = true;
	}

	void FactoryManager::shutdown()
	{
		MYGUI_ASSERT(mIsInitialise, getClassTypeName() << " is not initialised");
		MYGUI_LOG(Info, "* Shutdown: " << getClassTypeName());

		MYGUI_LOG(Info, getClassTypeName() << " successfully shutdown");
		mIsInitialise = false;
	}

	void FactoryManager::registerFactory(const std::string& _category, const std::string& _type, FactoryFunc* _delegate)
	{
		//FIXME
        MYGUI_LOG(Info, getClassTypeName() << " register factory " << _category << ":" << _type << " -> " << _delegate);
		mRegisterFactoryItems[_category][_type] = _delegate;
	}

	void FactoryManager::unregisterFactory(const std::string& _category, const std::string& _type)
	{
		MapRegisterFactoryItem::iterator category = mRegisterFactoryItems.find(_category);
		if (category == mRegisterFactoryItems.end())
		{
			return;
		}
		MapFactoryItem::iterator type = category->second.find(_type);
		if (type == category->second.end())
		{
			return;
		}

		category->second.erase(type);
	}

	void FactoryManager::unregisterFactory(const std::string& _category)
	{
		MapRegisterFactoryItem::iterator category = mRegisterFactoryItems.find(_category);
		if (category == mRegisterFactoryItems.end())
		{
			return;
		}
		mRegisterFactoryItems.erase(category);
	}

	IObject* FactoryManager::createObject(const std::string& _category, const std::string& _type)
	{
		MapRegisterFactoryItem::iterator category = mRegisterFactoryItems.find(_category);
		if (category == mRegisterFactoryItems.end())
		{
            MYGUI_LOG(Warning, getClassTypeName() << " not found category " << _category);
			return nullptr;
		}

		MapFactoryItem::iterator type = category->second.find(_type);
		if (type == category->second.end())
		{
            MYGUI_LOG(Warning, getClassTypeName() << " not found type " << _type << " for category " << _category);
			return nullptr;
		}
		if (!type->second)
		{
            MYGUI_LOG(Warning, getClassTypeName() << " factory " << _category << ":" << _type << " is empty");
			return nullptr;
		}

		IObject* result = nullptr;
		(type->second)(result);
		return result;
	}

	void FactoryManager::destroyObject(IObject* _object)
	{
		delete _object;

		/*MapRegisterFactoryItem::iterator category = mRegisterFactoryItems.find(_category);
		if (category == mRegisterFactoryItems.end())
		{
			return;
		}
		MapFactoryItem::iterator type = category->second.find(_type);
		if (type == category->second.end())
		{
			return;
		}
		if (type->second.empty())
		{
			return;
		}

		type->second(_object, nullptr, _version);*/
	}

	bool FactoryManager::isFactoryExist(const std::string& _category, const std::string& _type)
	{
		MapRegisterFactoryItem::iterator category = mRegisterFactoryItems.find(_category);
		if (category == mRegisterFactoryItems.end())
		{
			return false;
		}
		MapFactoryItem::iterator type = category->second.find(_type);
		if (type == category->second.end())
		{
			return false;
		}

		return true;
	}

} // namespace MyGUI
