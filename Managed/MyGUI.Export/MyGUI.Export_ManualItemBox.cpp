/*!
	@file
	@author		Generate utility by Albert Semenov
	@date		01/2009
	@module
*/

#include "ExportDefine.h"
#include "ExportMarshaling.h"
#include "ExportMarshalingWidget.h"
#include "ExportMarshalingType.h"
#include <MyGUI.h>

namespace Export
{

   	namespace ScopeItemBoxMethod_GetItemDataAt
	{
		MYGUIEXPORT Convert<MyGUI::Any>::Type MYGUICALL ExportItemBox_GetItemDataAt_index( MyGUI::Widget* _native,
			Convert<size_t>::Type _index )
		{
			Convert<MyGUI::Any>::Type* data = static_cast< MyGUI::ItemBox * >(_native)->getItemDataAt< Convert<MyGUI::Any>::Type >(
				Convert<size_t>::From( _index ), false );
			return data == nullptr ? nullptr : *data;
		}
	}

	namespace ScopeItemBoxMethod_GetIndexByWidget
	{
		MYGUIEXPORT Convert<size_t>::Type MYGUICALL ExportItemBox_GetIndexByWidget_widget( MyGUI::Widget* _native,
			Convert<MyGUI::Widget *>::Type _widget )
		{
			return Convert<size_t>::To( static_cast< MyGUI::ItemBox * >(_native)->getIndexByWidget(
				Convert<MyGUI::Widget *>::From( _widget ) ));
		}
	}

} // namespace Export
