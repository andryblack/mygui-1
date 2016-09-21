/*
 * This source file is part of MyGUI. For the latest info, see http://mygui.info/
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#include "MyGUI_Precompiled.h"
#include "MyGUI_EditText.h"
#include "MyGUI_RenderItem.h"
#include "MyGUI_FontManager.h"
#include "MyGUI_RenderManager.h"
#include "MyGUI_LanguageManager.h"
#include "MyGUI_TextIterator.h"
#include "MyGUI_IRenderTarget.h"
#include "MyGUI_FontData.h"
#include "MyGUI_CommonStateInfo.h"

namespace MyGUI
{

	const size_t VERTEX_IN_QUAD = 4;
	const size_t SIMPLETEXT_COUNT_VERTEX = 32 * VERTEX_IN_QUAD;

	EditText::EditText() :
		ISubWidgetText(),
		mEmptyView(false),
		mCurrentColourNative(0x00FFFFFF),
		mInverseColourNative(0x00000000),
		mCurrentAlphaNative(0xFF000000),
		mTextOutDate(false),
		mTextAlign(Align::Default),
		mColour(Colour::White),
		mAlpha(ALPHA_MAX),
		mFont(nullptr),
		mFontHeight(0),
		mBackgroundNormal(true),
		mCursorPosition(0),
		mVisibleCursor(false),
		mNode(nullptr),
		mRenderItem(nullptr),
		mCountVertex(SIMPLETEXT_COUNT_VERTEX),
		mIsAddCursorWidth(true),
		mShiftText(false),
		mWordWrap(false),
		mManualColour(false),
		mOldWidth(0)
	{
	
		mCurrentColourNative = texture_utility::toColourARGB(mColour);
	
		mCurrentColourNative = (mCurrentColourNative & 0x00FFFFFF) | (mCurrentAlphaNative & 0xFF000000);
		mInverseColourNative = mCurrentColourNative ^ 0x00FFFFFF;
	}

	EditText::~EditText()
	{
	}

	void EditText::setVisible(bool _visible)
	{
		if (mVisible == _visible)
			return;
		mVisible = _visible;

		if (nullptr != mNode)
			mNode->outOfDate(mRenderItem);
	}

	void EditText::_correctView()
	{
		if (nullptr != mNode)
			mNode->outOfDate(mRenderItem);
	}

	void EditText::_setAlign(const IntSize& _oldsize)
	{
		if (mWordWrap)
		{
			// передается старая координата всегда
			int width = mCroppedParent->getWidth();
			if (mOldWidth != width)
			{
				mOldWidth = width;
				mTextOutDate = true;
			}
		}

		// необходимо разобраться
		bool need_update = true;//_update;

		// первоначальное выравнивание
		if (mAlign.isHStretch())
		{
			// растягиваем
			mCoord.width = mCoord.width + (mCroppedParent->getWidth() - _oldsize.width);
			need_update = true;
			mIsMargin = true; // при изменении размеров все пересчитывать
		}
		else if (mAlign.isRight())
		{
			// двигаем по правому краю
			mCoord.left = mCoord.left + (mCroppedParent->getWidth() - _oldsize.width);
			need_update = true;
		}
		else if (mAlign.isHCenter())
		{
			// выравнивание по горизонтали без растяжения
			mCoord.left = (mCroppedParent->getWidth() - mCoord.width) / 2;
			need_update = true;
		}

		if (mAlign.isVStretch())
		{
			// растягиваем
			mCoord.height = mCoord.height + (mCroppedParent->getHeight() - _oldsize.height);
			need_update = true;
			mIsMargin = true; // при изменении размеров все пересчитывать
		}
		else if (mAlign.isBottom())
		{
			// двигаем по нижнему краю
			mCoord.top = mCoord.top + (mCroppedParent->getHeight() - _oldsize.height);
			need_update = true;
		}
		else if (mAlign.isVCenter())
		{
			// выравнивание по вертикали без растяжения
			mCoord.top = (mCroppedParent->getHeight() - mCoord.height) / 2;
			need_update = true;
		}

		if (need_update)
		{
			mCurrentCoord = mCoord;
			_updateView();
		}
	}

	void EditText::_updateView()
	{
		bool margin = _checkMargin();

		mEmptyView = ((0 >= _getViewWidth()) || (0 >= _getViewHeight()));

		mCurrentCoord.left = mCoord.left + mMargin.left;
		mCurrentCoord.top = mCoord.top + mMargin.top;

		// вьюпорт стал битым
		if (margin)
		{
			// проверка на полный выход за границу
			if (_checkOutside())
			{
				// запоминаем текущее состояние
				mIsMargin = margin;

				// обновить перед выходом
				if (nullptr != mNode)
					mNode->outOfDate(mRenderItem);
				return;
			}
		}

		// мы обрезаны или были обрезаны
		if (mIsMargin || margin)
		{
			mCurrentCoord.width = _getViewWidth();
			mCurrentCoord.height = _getViewHeight();
		}

		// запоминаем текущее состояние
		mIsMargin = margin;

		if (nullptr != mNode)
			mNode->outOfDate(mRenderItem);
	}

	void EditText::setCaption(const UString& _value)
	{
		mCaption = _value;
		mTextOutDate = true;

		checkVertexSize();

		if (nullptr != mNode)
			mNode->outOfDate(mRenderItem);
	}

	void EditText::checkVertexSize()
	{
		// если вершин не хватит, делаем реалок, с учетом выделения * 2 и курсора
		size_t need = (mCaption.length() * 2 + 2) * VERTEX_IN_QUAD;
		if (mCountVertex < need)
		{
			mCountVertex = need + SIMPLETEXT_COUNT_VERTEX;
		}
	}

	const UString& EditText::getCaption() const
	{
		return mCaption;
	}

	void EditText::setTextColour(const Colour& _value)
	{
		mManualColour = true;
		_setTextColour(_value);
	}

	void EditText::_setTextColour(const Colour& _value)
	{
		if (mColour == _value)
			return;

		mColour = _value;
		mCurrentColourNative = texture_utility::toColourARGB(mColour);

		mCurrentColourNative = (mCurrentColourNative & 0x00FFFFFF) | (mCurrentAlphaNative & 0xFF000000);
		mInverseColourNative = mCurrentColourNative ^ 0x00FFFFFF;

		if (nullptr != mNode)
			mNode->outOfDate(mRenderItem);
	}

	const Colour& EditText::getTextColour() const
	{
		return mColour;
	}

	void EditText::setAlpha(float _value)
	{
		if (mAlpha == _value)
			return;
		mAlpha = _value;

		mCurrentAlphaNative = ((uint8)(mAlpha * 255) << 24);
		mCurrentColourNative = (mCurrentColourNative & 0x00FFFFFF) | (mCurrentAlphaNative & 0xFF000000);
		mInverseColourNative = mCurrentColourNative ^ 0x00FFFFFF;

		if (nullptr != mNode)
			mNode->outOfDate(mRenderItem);
	}

	float EditText::getAlpha() const
	{
		return mAlpha;
	}

	void EditText::setFontName(const std::string& _value)
	{
		mFont = FontManager::getInstance().getByName(_value);
		if (mFont != nullptr)
		{
		
			// если надо, устанавливаем дефолтный размер шрифта
			if (mFont->getDefaultHeight() != 0)
			{
				mFontHeight = mFont->getDefaultHeight();
			}
		}

		mTextOutDate = true;

		// если мы были приаттаченны, то удаляем себя
		if (nullptr != mRenderItem)
		{
			mRenderItem->removeDrawItem(this);
			mRenderItem = nullptr;
		}

		// если есть текстура, то приаттачиваемся
		if (nullptr != mFont && nullptr != mNode)
		{
			mRenderItem = mNode->addToRenderItem(mFont, false, false);
			mRenderItem->addDrawItem(this);
		}

		if (nullptr != mNode)
			mNode->outOfDate(mRenderItem);
	}

	IFont* EditText::getFont() const
	{
		return mFont;
	}

	void EditText::setFontHeight(int _value)
	{
		mFontHeight = _value;
		mTextOutDate = true;

		if (nullptr != mNode)
			mNode->outOfDate(mRenderItem);
	}

	int EditText::getFontHeight() const
	{
		return mFontHeight;
	}

	void EditText::createDrawItem(ITexture* _texture, ILayerNode* _node)
	{
		mNode = _node;
		// если уже есть текстура, то атачимся, актуально для смены леера
		if (nullptr != mFont)
		{
			MYGUI_ASSERT(!mRenderItem, "mRenderItem must be nullptr");

			mRenderItem = mNode->addToRenderItem(mFont, false, false);
			mRenderItem->addDrawItem(this);
		}
	}

	void EditText::destroyDrawItem()
	{
		if (nullptr != mRenderItem)
		{
			mRenderItem->removeDrawItem(this);
			mRenderItem = nullptr;
		}
		mNode = nullptr;
	}

	bool EditText::isVisibleCursor() const
	{
		return mVisibleCursor;
	}

	void EditText::setVisibleCursor(bool _value)
	{
		if (mVisibleCursor == _value)
			return;
		mVisibleCursor = _value;

		if (nullptr != mNode)
			mNode->outOfDate(mRenderItem);
	}

	size_t EditText::getCursorPosition() const
	{
		return mCursorPosition;
	}

	void EditText::setCursorPosition(size_t _index)
	{
		if (mCursorPosition == _index)
			return;
		mCursorPosition = _index;

		if (nullptr != mNode)
			mNode->outOfDate(mRenderItem);
	}

	void EditText::setTextAlign(Align _value)
	{
		mTextAlign = _value;

		if (nullptr != mNode)
			mNode->outOfDate(mRenderItem);
	}

	Align EditText::getTextAlign() const
	{
		return mTextAlign;
	}

	IntSize EditText::getTextSize() const
	{
		// если нуно обновить, или изменились пропорции экрана
		if (mTextOutDate)
			const_cast<EditText*>(this)->updateRawData();

		IntSize size = mTextView.getViewSize();
		// плюс размер курсора
		if (mIsAddCursorWidth)
			size.width += 2;

		return size;
	}

	const VectorLineInfo& EditText::getLineInfo() const
	{
		return mTextView.getData();
	}

	void EditText::setViewOffset(const IntPoint& _point)
	{
		mViewOffset = _point;

		if (nullptr != mNode)
			mNode->outOfDate(mRenderItem);
	}

	IntPoint EditText::getViewOffset() const
	{
		return mViewOffset;
	}

	size_t EditText::getCursorPosition(const IntPoint& _point)
	{
		if (nullptr == mFont)
			return 0;

		if (mTextOutDate)
			updateRawData();

		IntPoint point = _point;
		point -= mCroppedParent->getAbsolutePosition();
		point += mViewOffset;
		point -= mCoord.point();

		return mTextView.getCursorPosition(point);
	}

	IntCoord EditText::getCursorCoord(size_t _position)
	{
		if (nullptr == mFont)
			return IntCoord();

		if (mTextOutDate)
			updateRawData();

		IntPoint point = mTextView.getCursorPoint(_position);
		point += mCroppedParent->getAbsolutePosition();
		point -= mViewOffset;
		point += mCoord.point();

		return IntCoord(point.left, point.top, 2, mFontHeight);
	}

	void EditText::setShiftText(bool _value)
	{
		if (mShiftText == _value)
			return;
		mShiftText = _value;

		if (nullptr != mNode)
			mNode->outOfDate(mRenderItem);
	}
    
    bool EditText::getWordWrap() const
    {
        return mWordWrap;
    }

	void EditText::setWordWrap(bool _value)
	{
		mWordWrap = _value;
		mTextOutDate = true;

		if (nullptr != mNode)
			mNode->outOfDate(mRenderItem);
	}

	void EditText::updateRawData()
	{
		if (nullptr == mFont)
			return;
		// сбрасывам флаги
		mTextOutDate = false;

		int width = -1;
		if (mWordWrap)
		{
			width = mCoord.width;
			// обрезать слова нужно по шарине, которую мы реально используем
			if (mIsAddCursorWidth)
				width -= 2;
		}

		mTextView.update(mCaption, mFont, mFontHeight, mTextAlign, width);
	}

	void EditText::setStateData(IStateInfo* _data)
	{
		EditTextStateInfo* data = _data->castType<EditTextStateInfo>();
		if (!mManualColour && data->getColour() != Colour::Zero)
			_setTextColour(data->getColour());
		setShiftText(data->getShift());
	}

	void EditText::doRender(IRenderTarget* _target)
	{
		if (nullptr == mFont || !mVisible || mEmptyView)
			return;

		if (mRenderItem->getCurrentUpdate() || mTextOutDate)
			updateRawData();

				// колличество отрисованных вершин
		size_t vertexCount = 0;

		// текущие цвета
		uint32 colour = mCurrentColourNative;
		
		const VectorLineInfo& textViewData = mTextView.getData();

		

		FloatRect vertexRect;

		
		size_t index = 0;
       
        
        for (size_t pass=0;pass<mFont->getNumPasses();++pass) {
            
            std::map<std::string,MyGUI::Colour>::const_iterator clrit = mPassColour.find(mFont->getPassName(pass));
            bool fixed_color = clrit != mPassColour.end();
            if (fixed_color) {
                MyGUI::Colour pass_colour = clrit->second;
                pass_colour.alpha *= mAlpha;
                colour = texture_utility::toColourARGB(pass_colour);
            } else {
                colour = mCurrentColourNative;
            }
            FloatSize offset = mFont->getOffset(pass);
            float  top = (float)(-mViewOffset.top + mCoord.top) + offset.height;
            for (VectorLineInfoLines::const_iterator line = textViewData.lines.begin(); line != textViewData.lines.end(); ++line)
            {
                float left = (float)(line->offset - mViewOffset.left + mCoord.left) + offset.width;
                
                for (VectorCharInfo::const_iterator sim = line->simbols.begin(); sim != line->simbols.end(); ++sim)
                {
                    if (sim->isColour())
                    {
                        if (!fixed_color) {
                            colour = sim->getColour() | (colour & 0xFF000000);
                        }
                        continue;
                    }
                    
                    
                    float fullAdvance = sim->getAdvance();
                    
                    
                    const GlyphInfo* info = mFont->getGlyphInfo(pass, sim->getChar());
                    if (info) {
                        vertexRect.left = left + (info->bearingX ) * textViewData.scale;
                        vertexRect.top = top + (info->bearingY ) * textViewData.scale;
                        vertexRect.right = vertexRect.left + info->width * textViewData.scale;
                        vertexRect.bottom = vertexRect.top + info->height * textViewData.scale;
                        _target->setTexture(info->texture);
                        drawGlyph(_target, vertexCount, vertexRect, info->uvRect, colour);
                    }
                    
                    
                    left += fullAdvance;
                    ++index;
                }
                
                top += mFontHeight;
                ++index;
            }
        }
        
		// Render the cursor, if any, last.
		if (mVisibleCursor)
		{
			IntPoint point = mTextView.getCursorPoint(mCursorPosition) - mViewOffset + mCoord.point();
			GlyphInfo* cursorGlyph = mFont->getGlyphInfo(-1,static_cast<Char>(FontCodeType::Cursor));
			vertexRect.set((float)point.left, (float)point.top, (float)point.left + cursorGlyph->width, (float)(point.top + mFontHeight));
            _target->setTexture(cursorGlyph->texture);
			drawGlyph(_target, vertexCount, vertexRect, cursorGlyph->uvRect, mCurrentColourNative | 0x00FFFFFF);
		}

	}

    void EditText::setPassColour(const std::string& pass,const Colour& _value)
	{
        mPassColour[pass] = _value;
		
		if (nullptr != mNode)
			mNode->outOfDate(mRenderItem);
	}

	void EditText::drawGlyph(
		IRenderTarget* _target,
		size_t& _vertexCount,
		FloatRect _vertexRect,
		FloatRect _textureRect,
		uint32 _colour) const
	{
		// символ залазиет влево
		float leftClip = (float)mCurrentCoord.left - _vertexRect.left;
		if (leftClip > 0.0f)
		{
			if ((float)mCurrentCoord.left < _vertexRect.right)
			{
				_textureRect.left += _textureRect.width() * leftClip / _vertexRect.width();
				_vertexRect.left += leftClip;
			}
			else
			{
				return;
			}
		}

		// символ залазиет вправо
		float rightClip = _vertexRect.right - (float)mCurrentCoord.right();
		if (rightClip > 0.0f)
		{
			if (_vertexRect.left < (float)mCurrentCoord.right())
			{
				_textureRect.right -= _textureRect.width() * rightClip / _vertexRect.width();
				_vertexRect.right -= rightClip;
			}
			else
			{
				return;
			}
		}

		// символ залазиет вверх
		float topClip = (float)mCurrentCoord.top - _vertexRect.top;
		if (topClip > 0.0f)
		{
			if ((float)mCurrentCoord.top < _vertexRect.bottom)
			{
				_textureRect.top += _textureRect.height() * topClip / _vertexRect.height();
				_vertexRect.top += topClip;
			}
			else
			{
				return;
			}
		}

		// символ залазиет вниз
		float bottomClip = _vertexRect.bottom - (float)mCurrentCoord.bottom();
		if (bottomClip > 0.0f)
		{
			if (_vertexRect.top < (float)mCurrentCoord.bottom())
			{
				_textureRect.bottom -= _textureRect.height() * bottomClip / _vertexRect.height();
				_vertexRect.bottom -= bottomClip;
			}
			else
			{
				return;
			}
		}

		float pix_left = mCroppedParent->getAbsoluteLeft() + _vertexRect.left;
		float pix_top = mCroppedParent->getAbsoluteTop() + (mShiftText ? 1.0f : 0.0f) + _vertexRect.top;

        VertexQuad quad;
        quad.set(pix_left, pix_top, pix_left+_vertexRect.width(), pix_top+_vertexRect.height(),
                 mNode->getNodeDepth(),
                 _textureRect.left, _textureRect.top, _textureRect.right, _textureRect.bottom,
                 _colour);
        _target->addQuad(quad);
        _vertexCount += VERTEX_IN_QUAD;
	}

} // namespace MyGUI
