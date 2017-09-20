/*
 * This source file is part of MyGUI. For the latest info, see http://mygui.info/
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#include "MyGUI_Precompiled.h"
#include "MyGUI_TextIterator.h"
#include "MyGUI_FontData.h"

namespace MyGUI
{

	
	// возвращает текст без тегов
	UString TextIterator::getOnlyText(const UString& _text)
	{
		UString ret;
		ret.reserve(_text.length());

		UString::iterator end = _text.end();
		for (UString::iterator iter = _text.begin(); iter != end; ++iter)
		{

			if ((*iter) == L'#')
			{
				// следующий символ
				++ iter;
				if (iter == end) break;

				// тэг цвета
				if ((*iter) != L'#')
				{
					// остальные 5 символов цвета
					for (size_t pos = 0; pos < 5; pos++)
					{
						++ iter;
						if (iter == end)
						{
                            return ret;
						}
					}
					continue;
				}
			}

			// обыкновенный символ
			ret.push_back(*iter);
		}

		return ret;
	}

    UString TextIterator::getTextCharInfo(Char _char)
	{
		if (_char == L'#') return "##";
        UString::unicode_char buff[2] = {UString::unicode_char(_char),0};
		return UString(buff);
	}

	UString TextIterator::convertTagColour(const Colour& _colour)
	{
		const size_t SIZE = 16;
		char buff[SIZE];
		snprintf(buff, SIZE, "#%.2X%.2X%.2X", (int)(_colour.red * 255), (int)(_colour.green * 255), (int)(_colour.blue * 255));
		return buff;
	}

	UString TextIterator::toTagsString(const UString& _text)
	{
		// преобразуем в строку с тегами
		UString text(_text);
		for (UString::iterator iter = text.begin(); iter != text.end(); )
		{
			// потом переделать через TextIterator чтобы отвязать понятие тег от эдита
			if (L'#' == (*iter)) iter = text.insert(++iter, L'#');
            else ++iter;
		}
		return text;
	}

	void TextIterator::cutMaxLength(UString& _text, size_t _max)
	{
        if (_text.length() <= _max) return;
        size_t pos = 0;
		for (UString::iterator iter = _text.begin(); iter != _text.end(); ++iter)
		{
			// проверяем и обрезаем
			if (pos == _max)
			{
                _text.erase(iter,_text.end());
				return;
			}
            ++pos;
		}
	}

	void TextIterator::cutMaxLengthFromBeginning(UString& _text,size_t _max)
	{
		// узнаем размер без тегов
        size_t size = _text.length();
		if (size <= _max) return;

		// разница
		size_t diff = size - _max;

        // теперь пройдем от начала и узнаем реальную позицию разницы
		UString::iterator iter = _text.begin();
		for (; iter != _text.end(); ++iter)
		{
			// обычный символ был
			if (diff == 0) break;
			-- diff;
		}
        _text.erase(_text.begin(),iter);
	}

	UString TextIterator::getTextNewLine()
	{
		return "\n";
	}

	void TextIterator::normaliseNewLine(UString& _text,bool _remove)
	{
        UString::iterator it = _text.begin();
        UString::iterator prev = it;
        while (it != _text.end()) {
            if (*it == FontCodeType::LF) {
                if (*prev == FontCodeType::CR) {
                    if (_remove) {
                        ++it;
                        it = prev = _text.erase(prev,it);
                    } else {
                        it = prev = _text.erase(prev,it);
                    }
                }
            } else {
                ++it;
            }
        }
	}

} // namespace MyGUI
