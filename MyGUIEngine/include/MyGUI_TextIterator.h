/*
 * This source file is part of MyGUI. For the latest info, see http://mygui.info/
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#ifndef MYGUI_TEXT_ITERATOR_H_
#define MYGUI_TEXT_ITERATOR_H_

#include "MyGUI_Prerequest.h"
#include "MyGUI_Colour.h"

namespace MyGUI
{

	class MYGUI_EXPORT TextIterator
	{
	private:
		TextIterator();

	public:

		// возвращает текст без тегов
		static UString getOnlyText(const UString& _text);

		static UString getTextNewLine();

		static UString getTextCharInfo(Char _char);

		// просто конвертируем цвет в строку
		static UString convertTagColour(const Colour& _colour);

		static UString toTagsString(const UString& _text);

        static void cutMaxLengthFromBeginning(UString& _text,size_t length);
        static void cutMaxLength(UString& _text,size_t length);
        static void normaliseNewLine(UString& _text,bool _remove);
        
	};

} // namespace MyGUI

#endif // MYGUI_TEXT_ITERATOR_H_
