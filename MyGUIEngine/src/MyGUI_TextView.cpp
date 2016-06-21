/*
 * This source file is part of MyGUI. For the latest info, see http://mygui.info/
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#include "MyGUI_Precompiled.h"
#include "MyGUI_TextView.h"

namespace MyGUI
{

	namespace
	{

		template<typename T>
		void setMin(T& _var, const T& _newValue)
		{
			if (_newValue < _var)
				_var = _newValue;
		}

		template<typename T>
		void setMax(T& _var, const T& _newValue)
		{
			if (_var < _newValue)
				_var = _newValue;
		}

	}

	class RollBackPoint
	{
	public:
		RollBackPoint() :
			position(0),
			count(0),
			width(0),
			rollback(false)
		{
		}

		void set(size_t _position, UString::const_iterator& _space_point, size_t _count, float _width)
		{
			position = _position;
			space_point = _space_point;
			count = _count;
			width = _width;
			rollback = true;
		}

		void clear()
		{
			rollback = false;
		}

		bool empty() const
		{
			return !rollback;
		}

		float getWidth() const
		{
			MYGUI_DEBUG_ASSERT(rollback, "rollback point not valid");
			return width;
		}

		size_t getCount() const
		{
			MYGUI_DEBUG_ASSERT(rollback, "rollback point not valid");
			return count;
		}

		size_t getPosition() const
		{
			MYGUI_DEBUG_ASSERT(rollback, "rollback point not valid");
			return position;
		}

		UString::const_iterator getTextIter() const
		{
			MYGUI_DEBUG_ASSERT(rollback, "rollback point not valid");
			return space_point;
		}

	private:
		size_t position;
		UString::const_iterator space_point;
		size_t count;
		float width;
		bool rollback;
	};

	TextView::TextView() :
		mLength(0),
		mFontHeight(0)
	{
	}

	void TextView::update(const UString& _text, IFont* _font, int _height, Align _align, int _maxWidth)
	{
		mFontHeight = _height;

		// массив для быстрой конвертации цветов
		static const char convert_colour[64] =
		{
			0,  1,  2,  3,  4,  5,  6, 7, 8, 9, 0, 0, 0, 0, 0, 0,
			0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0
		};

		mViewSize.clear();

		RollBackPoint roll_back;
		IntSize result;
		float width = 0.0f;
		size_t count = 0;
		mLength = 0;
		mLineInfo.clear();
		LineInfo line_info;
		int font_height = _font->getDefaultHeight();
        
        if (_height != font_height) {
            mLineInfo.scale = (float)_height / font_height;
        }

		UString::const_iterator end = _text.end();
		UString::const_iterator index = _text.begin();

		/*if (index == end)
			return;*/

		result.height += _height;

		for (; index != end; ++index)
		{
			Char character = *index;

			// новая строка
			if (character == FontCodeType::CR
				|| character == FontCodeType::NEL
				|| character == FontCodeType::LF)
			{
				if (character == FontCodeType::CR)
				{
					UString::const_iterator peeki = index;
					++peeki;
					if ((peeki != end) && (*peeki == FontCodeType::LF))
						index = peeki; // skip both as one newline
				}

				line_info.width = (int)ceil(width);
				line_info.count = count;
				mLength += line_info.count + 1;

				result.height += _height;
				
				width = 0;
				count = 0;

				mLineInfo.lines.push_back(line_info);
				line_info.clear();

				// отменяем откат
				roll_back.clear();

				continue;
			}
			// тег
			else if (character == L'#')
			{
				// берем следующий символ
				++ index;
				if (index == end)
				{
					--index;    // это защита
					continue;
				}

				character = *index;
				// если два подряд, то рисуем один шарп, если нет то меняем цвет
				if (character != L'#')
				{
					// парсим первый символ
					uint32 colour = convert_colour[(character - 48) & 0x3F];

					// и еще пять символов после шарпа
					for (char i = 0; i < 5; i++)
					{
						++ index;
						if (index == end)
						{
							--index;    // это защита
							continue;
						}
						colour <<= 4;
						colour += convert_colour[ ((*index) - 48) & 0x3F ];
					}
                    colour = ((colour & 0x00ff0000) >> 16) |
                             ((colour & 0x000000ff) << 16) |
                             ((colour & 0xff00ff00));
					line_info.simbols.push_back( CharInfo(colour) );

					continue;
				}
			}

			GlyphInfo* info = _font->getGlyphInfo(-1,character);

			if (info == nullptr)
				continue;

			if (FontCodeType::Space == character)
			{
				roll_back.set(line_info.simbols.size(), index, count, width);
			}
			else if (FontCodeType::Tab == character)
			{
				roll_back.set(line_info.simbols.size(), index, count, width);
			}

			float char_advance = info->advance * mLineInfo.scale;
			
			float char_fullAdvance = char_advance;

			// перенос слов
			if (_maxWidth != -1
				&& (width + char_fullAdvance) > _maxWidth
				&& !roll_back.empty())
			{
				// откатываем до последнего пробела
				width = roll_back.getWidth();
				count = roll_back.getCount();
				index = roll_back.getTextIter();
				line_info.simbols.erase(line_info.simbols.begin() + roll_back.getPosition(), line_info.simbols.end());

				// запоминаем место отката, как полную строку
				line_info.width = (int)ceil(width);
				line_info.count = count;
				mLength += line_info.count + 1;

				result.height += _height;
				
				width = 0;
				count = 0;

				mLineInfo.lines.push_back(line_info);
				line_info.clear();

				// отменяем откат
				roll_back.clear();

				continue;
			}

			line_info.simbols.push_back(CharInfo(character,char_fullAdvance));
			width += char_fullAdvance;
			count ++;
		}

		line_info.width = (int)ceil(width);
		line_info.count = count;
		mLength += line_info.count;

		mLineInfo.lines.push_back(line_info);
        result.width = 0.0f;
        float offset = 0.0f;
        for (VectorLineInfoLines::iterator line = mLineInfo.lines.begin(); line != mLineInfo.lines.end(); ++line)
        {
            line->offset = 0;
            width = line->width;
            if (!line->simbols.empty()) {
                for (VectorCharInfo::const_iterator simbol = line->simbols.begin();simbol!=line->simbols.end();++simbol) {
                    if (!simbol->isColour()) {
                        const GlyphInfo* info = _font->getGlyphInfo(-1, simbol->getChar());
                        if (info) {
                            if (info->bearingX < 0) {
                                float firstOffset = info->bearingX * mLineInfo.scale;
                                setMax(offset, firstOffset);
                                width += firstOffset;
                            }
                        }
                        break;
                    }
                }
                for (VectorCharInfo::const_reverse_iterator simbol = line->simbols.rbegin();simbol!=line->simbols.rend();++simbol) {
                    if (!simbol->isColour()) {
                        const GlyphInfo* info = _font->getGlyphInfo(-1, simbol->getChar());
                        if (info) {
                            float rightOffset = ((info->bearingX + info->width) - info->advance) * mLineInfo.scale;
                            if (rightOffset > 0) {
                                width += rightOffset;
                            }
                        }
                        break;
                    }
                }
            }
            line->width = (int)ceil(width);
            setMax(result.width, line_info.width);
        }
        
        if (!mLineInfo.lines.empty()) {
            int addHeight = 0;
            for (VectorCharInfo::const_reverse_iterator simbol = mLineInfo.lines.back().simbols.rbegin();simbol!=mLineInfo.lines.back().simbols.rend();++simbol) {
                if (!simbol->isColour()) {
                    const GlyphInfo* info = _font->getGlyphInfo(-1, simbol->getChar());
                    if (info) {
                        int bottomOffset = int(ceil(((info->bearingY + info->height) * mLineInfo.scale - _height)));
                        if (bottomOffset > 0) {
                            setMax(addHeight, bottomOffset);
                        }
                    }
                    break;
                }
            }
            
            result.height += addHeight;
        }
        
		
		// теперь выравниванием строки
		for (VectorLineInfoLines::iterator line = mLineInfo.lines.begin(); line != mLineInfo.lines.end(); ++line)
		{
            line->offset = offset;
			if (_align.isRight())
				line->offset += result.width - line->width;
			else if (_align.isHCenter())
				line->offset += (result.width - line->width) / 2;
		}
        

		mViewSize = result;
	}
    
   

	size_t TextView::getCursorPosition(const IntPoint& _value)
	{
		const int height = mFontHeight;
		size_t result = 0;
		int top = 0;

		for (VectorLineInfoLines::const_iterator line = mLineInfo.lines.begin(); line != mLineInfo.lines.end(); ++line)
		{
			// это последняя строка
			bool lastline = !(line + 1 != mLineInfo.lines.end());

			// наша строчка
			if (top + height > _value.top || lastline)
			{
				top += height;
				float left = (float)line->offset;
				int count = 0;

				// ищем символ
				for (VectorCharInfo::const_iterator sim = line->simbols.begin(); sim != line->simbols.end(); ++sim)
				{
					if (sim->isColour())
						continue;

					float fullAdvance = sim->getAdvance();
					if (left + fullAdvance / 2.0f > _value.left)
					{
						break;
					}
					left += fullAdvance;
					count ++;
				}

				result += count;
				break;
			}

			if (!lastline)
			{
				top += height;
				result += line->count + 1;
			}
		}

		return result;
	}

	IntPoint TextView::getCursorPoint(size_t _position)
	{
		setMin(_position, mLength);

		size_t position = 0;
		int top = 0;
		float left = 0.0f;
		for (VectorLineInfoLines::const_iterator line = mLineInfo.lines.begin(); line != mLineInfo.lines.end(); ++line)
		{
			left = (float)line->offset;
			if (position + line->count >= _position)
			{
				for (VectorCharInfo::const_iterator sim = line->simbols.begin(); sim != line->simbols.end(); ++sim)
				{
					if (sim->isColour())
						continue;

					if (position == _position)
						break;

					position ++;
					left += sim->getAdvance();
				}
				break;
			}
			position += line->count + 1;
			top += mFontHeight;
		}

		return IntPoint((int)left, top);
	}

	const IntSize& TextView::getViewSize() const
	{
		return mViewSize;
	}

	size_t TextView::getTextLength() const
	{
		return mLength;
	}

	const VectorLineInfo& TextView::getData() const
	{
		return mLineInfo;
	}

} // namespace MyGUI
