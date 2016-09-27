/*
 * This source file is part of MyGUI. For the latest info, see http://mygui.info/
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#include "MyGUI_Precompiled.h"
#include "MyGUI_EditBox.h"
#include "MyGUI_Gui.h"
#include "MyGUI_ResourceSkin.h"
#include "MyGUI_SkinManager.h"
#include "MyGUI_InputManager.h"
#include "MyGUI_ClipboardManager.h"
#include "MyGUI_ISubWidgetText.h"
#include "MyGUI_ScrollBar.h"

#include <ctype.h>

namespace MyGUI
{

	const float EDIT_CURSOR_TIMER  = 0.7f;
	const float EDIT_ACTION_MOUSE_TIMER  = 0.05f;
	const int EDIT_CURSOR_MAX_POSITION = 100000;
	const int EDIT_CURSOR_MIN_POSITION = -100000;
	const size_t EDIT_MAX_UNDO = 128;
	const size_t EDIT_DEFAULT_MAX_TEXT_LENGTH = 2048;
	const float EDIT_OFFSET_HORZ_CURSOR = 10.0f; // дополнительное смещение для курсора
	const int EDIT_ACTION_MOUSE_ZONE = 1500; // область для восприятия мыши за пределом эдита
	const std::string EDIT_CLIPBOARD_TYPE_TEXT = "Text";
	const int EDIT_MOUSE_WHEEL = 50; // область для восприятия мыши за пределом эдита

	EditBox::EditBox() :
		mIsPressed(false),
		mIsFocus(false),
		mCursorActive(false),
		mCursorTimer(0),
		mActionMouseTimer(0),
		mCursorPosition(0),
		mTextLength(0),
		mMouseLeftPressed(false),
		mModeReadOnly(false),
		mModePassword(false),
		mModeMultiline(false),
		mModeStatic(false),
		mModeWordWrap(false),
		mTabPrinting(false),
		mCharPassword('*'),
		mOverflowToTheLeft(false),
		mMaxTextLength(EDIT_DEFAULT_MAX_TEXT_LENGTH),
		mClientText(nullptr)
	{
		mChangeContentByResize = true;
	}

	void EditBox::initialiseOverride()
	{
		Base::initialiseOverride();

		mOriginalPointer = getPointer();

		// FIXME нам нужен фокус клавы
		setNeedKeyFocus(true);

		///@wskin_child{EditBox, Widget, Client} Клиентская зона.
		assignWidget(mClient, "Client");
		if (mClient != nullptr)
		{
			mClient->eventMouseSetFocus += newDelegate(this, &EditBox::notifyMouseSetFocus);
			mClient->eventMouseLostFocus += newDelegate(this, &EditBox::notifyMouseLostFocus);
			mClient->eventMouseButtonPressed += newDelegate(this, &EditBox::notifyMousePressed);
			mClient->eventMouseButtonReleased += newDelegate(this, &EditBox::notifyMouseReleased);
			mClient->eventMouseDrag += newDelegate(this, &EditBox::notifyMouseDrag);
			mClient->eventMouseButtonDoubleClick += newDelegate(this, &EditBox::notifyMouseButtonDoubleClick);
			mClient->eventMouseWheel += newDelegate(this, &EditBox::notifyMouseWheel);
			setWidgetClient(mClient);
		}

		///@wskin_child{EditBox, ScrollBar, VScroll} Вертикальная полоса прокрутки.
		assignWidget(mVScroll, "VScroll");
		if (mVScroll != nullptr)
		{
			mVScroll->eventScrollChangePosition += newDelegate(this, &EditBox::notifyScrollChangePosition);
		}

		///@wskin_child{EditBox, ScrollBar, HScroll} Горизонтальная полоса прокрутки.
		assignWidget(mHScroll, "HScroll");
		if (mHScroll != nullptr)
		{
			mHScroll->eventScrollChangePosition += newDelegate(this, &EditBox::notifyScrollChangePosition);
		}

		mClientText = getSubWidgetText();
		if (mClient != nullptr)
		{
			ISubWidgetText* text = mClient->getSubWidgetText();
			if (text)
				mClientText = text;
		}

		updateScrollSize();

		// первоначальная инициализация курсора
		if (mClientText != nullptr)
			mClientText->setCursorPosition(mCursorPosition);

        updateViewWithCursor();
	}

	void EditBox::shutdownOverride()
	{
		mClient = nullptr;
		mClientText = nullptr;
		mVScroll = nullptr;
		mHScroll = nullptr;

		Base::shutdownOverride();
	}

	void EditBox::notifyMouseSetFocus(Widget* _sender, Widget* _old)
	{
		if ((_old == mClient) || (mIsFocus))
			return;

		mIsFocus = true;
		updateEditState();
	}

	void EditBox::notifyMouseLostFocus(Widget* _sender, Widget* _new)
	{
		if ((_new == mClient) || (!mIsFocus))
			return;

		mIsFocus = false;
		updateEditState();
	}

	void EditBox::notifyMousePressed(Widget* _sender, int _left, int _top, MouseButton _id)
	{
		if (mClientText == nullptr)
			return;

		// в статике все недоступно
		if (mModeStatic)
			return;

		IntPoint point = InputManager::getInstance().getLastPressedPosition(MouseButton::Left);
		mCursorPosition = mClientText->getCursorPosition(point);
		mClientText->setCursorPosition(mCursorPosition);
		mClientText->setVisibleCursor(true);
		mCursorTimer = 0;
		updateViewWithCursor();

		if (_id == MouseButton::Left)
			mMouseLeftPressed = true;
	}

	void EditBox::notifyMouseReleased(Widget* _sender, int _left, int _top, MouseButton _id)
	{
		// сбрасываем всегда
		mMouseLeftPressed = false;
	}

	void EditBox::notifyMouseDrag(Widget* _sender, int _left, int _top, MouseButton _id)
	{
		if (_id != MouseButton::Left)
			return;

		if (mClientText == nullptr)
			return;

		// в статике все недоступно
		if (mModeStatic)
			return;

		// останавливаем курсор
		mClientText->setVisibleCursor(true);

		// сбрасываем все таймеры
		mCursorTimer = 0;
		mActionMouseTimer = 0;

		size_t Old = mCursorPosition;
		IntPoint point(_left, _top);
		mCursorPosition = mClientText->getCursorPosition(point);
		if (Old == mCursorPosition)
			return;

		mClientText->setCursorPosition(mCursorPosition);

	}

	void EditBox::notifyMouseButtonDoubleClick(Widget* _sender)
	{
		if (mClientText == nullptr)
			return;

		// в статике все недоступно
		if (mModeStatic)
			return;

		const IntPoint& lastPressed = InputManager::getInstance().getLastPressedPosition(MouseButton::Left);

		size_t cursorPosition = mClientText->getCursorPosition(lastPressed);
		mClientText->setCursorPosition(cursorPosition);
	}

	void EditBox::onMouseDrag(int _left, int _top, MouseButton _id)
	{
		notifyMouseDrag(nullptr, _left, _top, _id);

		Base::onMouseDrag(_left, _top, _id);
	}

	void EditBox::onKeySetFocus(Widget* _old)
	{
		if (!mIsPressed)
		{
			mIsPressed = true;
			updateEditState();

			if (!mModeStatic)
			{
				if (mClientText != nullptr)
				{
					mCursorActive = true;
					Gui::getInstance().eventFrameStart += newDelegate(this, &EditBox::frameEntered);
					mClientText->setVisibleCursor(true);
					mCursorTimer = 0;
				}
			}
		}

		Base::onKeySetFocus(_old);
	}

	void EditBox::onKeyLostFocus(Widget* _new)
	{
		if (mIsPressed)
		{
			mIsPressed = false;
			updateEditState();

			if (mClientText != nullptr)
			{
				mCursorActive = false;
				Gui::getInstance().eventFrameStart -= newDelegate(this, &EditBox::frameEntered);
				mClientText->setVisibleCursor(false);
			}
		}

		Base::onKeyLostFocus(_new);
	}

	void EditBox::onKeyButtonPressed(KeyCode _key, Char _char)
	{
		if (mClientText == nullptr || mClient == nullptr)
		{
			Base::onKeyButtonPressed(_key, _char);
			return;
		}

		// в статическом режиме ничего не доступно
		if (mModeStatic)
		{
			Base::onKeyButtonPressed(_key, _char);
			return;
		}

		InputManager& input = InputManager::getInstance();

		mClientText->setVisibleCursor(true);
		mCursorTimer = 0.0f;

		if (_key == KeyCode::Escape)
		{
			InputManager::getInstance().setKeyFocusWidget(nullptr);
		}
		else if (_key == KeyCode::Backspace)
		{
			// если нуно то удаляем выделенный текст
			if (!mModeReadOnly)
			{
				
                // прыгаем на одну назад и удаляем
                if (mCursorPosition != 0)
                {
                    mCursorPosition--;
                    eraseText(mCursorPosition, 1, true);
                }
				// отсылаем событие о изменении
				eventEditTextChange(this);
			}

		}
		else if (_key == KeyCode::Delete)
		{
			if (!mModeReadOnly)
			{
			
                if (mCursorPosition != mTextLength)
                {
                    eraseText(mCursorPosition, 1, true);
                }
				// отсылаем событие о изменении
				eventEditTextChange(this);
			}

		}
		else if (_key == KeyCode::Insert)
		{
			if (input.isShiftPressed())
			{
                commandPast();
			}
			else if (input.isControlPressed())
			{
				commandCopy();
			}

		}
		else if ((_key == KeyCode::Return) || (_key == KeyCode::NumpadEnter))
		{
			// работаем только в режиме редактирования
			if (!mModeReadOnly)
			{
				if ((mModeMultiline) && (!input.isControlPressed()))
				{
                    insertText(TextIterator::getTextNewLine(), mCursorPosition, true);
					// отсылаем событие о изменении
					eventEditTextChange(this);
				}
				// при сингл лайн и и мульти+сонтрол шлем эвент
				else
				{
					eventEditSelectAccept(this);
				}
			}

		}
		else if (_key == KeyCode::ArrowRight)
		{
			if ((mCursorPosition) < mTextLength)
			{
				mCursorPosition ++;
				mClientText->setCursorPosition(mCursorPosition);
                updateViewWithCursor();
			}

		}
		else if (_key == KeyCode::ArrowLeft)
		{
			if (mCursorPosition != 0)
			{
				mCursorPosition --;
				mClientText->setCursorPosition(mCursorPosition);
				updateViewWithCursor();
			}
        }
		else if (_key == KeyCode::ArrowUp)
		{
			IntPoint point = mClientText->getCursorPoint(mCursorPosition);
			point.top -= mClientText->getFontHeight();
			size_t old = mCursorPosition;
			mCursorPosition = mClientText->getCursorPosition(point);
			// самая верхняя строчка
			if (old == mCursorPosition)
			{
				if (mCursorPosition != 0)
				{
					mCursorPosition = 0;
					mClientText->setCursorPosition(mCursorPosition);
                    updateViewWithCursor();
				}
			}
			else
			{
				mClientText->setCursorPosition(mCursorPosition);
				updateViewWithCursor();
			}

		}
		else if (_key == KeyCode::ArrowDown)
		{
			IntPoint point = mClientText->getCursorPoint(mCursorPosition);
			point.top += mClientText->getFontHeight();
			size_t old = mCursorPosition;
			mCursorPosition = mClientText->getCursorPosition(point);
			// самая нижняя строчка
			if (old == mCursorPosition)
			{
				if (mCursorPosition != mTextLength)
				{
					mCursorPosition = mTextLength;
					mClientText->setCursorPosition(mCursorPosition);
                    updateViewWithCursor();
				}
			}
			else
			{
				mClientText->setCursorPosition(mCursorPosition);
				updateViewWithCursor();
			}

		}
		else if (_key == KeyCode::Home)
		{
			// в начало строки
			if (!input.isControlPressed())
			{
				IntPoint point = mClientText->getCursorPoint(mCursorPosition);
				point.left = EDIT_CURSOR_MIN_POSITION;
				size_t old = mCursorPosition;
				mCursorPosition = mClientText->getCursorPosition(point);
				if (old != mCursorPosition)
				{
					mClientText->setCursorPosition(mCursorPosition);
					updateViewWithCursor();
				}
			}
			// в начало всего текста
			else
			{
				if (0 != mCursorPosition)
				{
					mCursorPosition = 0;
					mClientText->setCursorPosition(mCursorPosition);
					updateViewWithCursor();
				}
			}

		}
		else if (_key == KeyCode::End)
		{
			// в конец строки
			if (!input.isControlPressed())
			{
				IntPoint point = mClientText->getCursorPoint(mCursorPosition);
				point.left = EDIT_CURSOR_MAX_POSITION;
				size_t old = mCursorPosition;
				mCursorPosition = mClientText->getCursorPosition(point);
				if (old != mCursorPosition)
				{
					mClientText->setCursorPosition(mCursorPosition);
					updateViewWithCursor();
				}
			}
			// в самый конец
			else
			{
				if (mTextLength != mCursorPosition)
				{
					mCursorPosition = mTextLength;
					mClientText->setCursorPosition(mCursorPosition);
					updateViewWithCursor();
				}
			}

		}
		else if (_key == KeyCode::PageUp)
		{
			// на размер окна, но не меньше одной строки
			IntPoint point = mClientText->getCursorPoint(mCursorPosition);
			point.top -= (mClient->getHeight() > mClientText->getFontHeight()) ? mClient->getHeight() : mClientText->getFontHeight();
			size_t old = mCursorPosition;
			mCursorPosition = mClientText->getCursorPosition(point);
			// самая верхняя строчка
			if (old == mCursorPosition)
			{
				if (mCursorPosition != 0)
				{
					mCursorPosition = 0;
					mClientText->setCursorPosition(mCursorPosition);
					updateViewWithCursor();
				}
			}
			else
			{
				mClientText->setCursorPosition(mCursorPosition);
				updateViewWithCursor();
            }

		}
		else if (_key == KeyCode::PageDown)
		{
			// на размер окна, но не меньше одной строки
			IntPoint point = mClientText->getCursorPoint(mCursorPosition);
			point.top += (mClient->getHeight() > mClientText->getFontHeight()) ? mClient->getHeight() : mClientText->getFontHeight();
			size_t old = mCursorPosition;
			mCursorPosition = mClientText->getCursorPosition(point);
			// самая нижняя строчка
			if (old == mCursorPosition)
			{
				if (mCursorPosition != mTextLength)
				{
					mCursorPosition = mTextLength;
					mClientText->setCursorPosition(mCursorPosition);
					updateViewWithCursor();
				}
			}
			else
			{
				mClientText->setCursorPosition(mCursorPosition);
				updateViewWithCursor();
			}

		}
		else if ((_key == KeyCode::LeftShift) || (_key == KeyCode::RightShift))
		{
		
        }
		else
		{
			// если не нажат контрл, то обрабатываем как текст
			if (!input.isControlPressed())
			{
				if (!mModeReadOnly && _char != 0)
				{
                    // таб только если нужно
					if (_char != '\t' || mTabPrinting)
					{
                        if (checkCharInFont(_char)) {
                            insertText(TextIterator::getTextCharInfo(_char), mCursorPosition, true);
                            // отсылаем событие о изменении
                            eventEditTextChange(this);
                        }
					}
				}
			}
			else if (_key == KeyCode::C)
			{
				commandCopy();

			}
            else if (_key == KeyCode::V)
			{
				commandPast();
			}
		}

		Base::onKeyButtonPressed(_key, _char);
	}

	void EditBox::frameEntered(float _frame)
	{
		if (mClientText == nullptr)
			return;

		// в статике все недоступно
		if (mModeStatic)
			return;

		if (mCursorActive)
		{
			mCursorTimer += _frame;

			if (mCursorTimer > EDIT_CURSOR_TIMER)
			{
				mClientText->setVisibleCursor(!mClientText->isVisibleCursor());
				while (mCursorTimer > EDIT_CURSOR_TIMER)
					mCursorTimer -= EDIT_CURSOR_TIMER;
			}
		}

		// сдвигаем курсор по положению мыши
		if (mMouseLeftPressed)
		{
			mActionMouseTimer += _frame;

			if (mActionMouseTimer > EDIT_ACTION_MOUSE_TIMER)
			{
				IntPoint mouse = InputManager::getInstance().getMousePositionByLayer();
				const IntRect& view = mClient->getAbsoluteRect();
				mouse.left -= view.left;
				mouse.top -= view.top;
				IntPoint point;

				bool action = false;

				// вверх на одну строчку
				if ((mouse.top < 0) && (mouse.top > -EDIT_ACTION_MOUSE_ZONE))
				{
					if ((mouse.left > 0) && (mouse.left <= mClient->getWidth()))
					{
						point = mClientText->getCursorPoint(mCursorPosition);
						point.top -= mClientText->getFontHeight();
						action = true;
					}
				}
				// вниз на одну строчку
				else if ((mouse.top > mClient->getHeight()) && (mouse.top < (mClient->getHeight() + EDIT_ACTION_MOUSE_ZONE)))
				{
					if ((mouse.left > 0) && (mouse.left <= mClient->getWidth()))
					{
						point = mClientText->getCursorPoint(mCursorPosition);
						point.top += mClientText->getFontHeight();
						action = true;
					}
				}

				// влево на небольшое расстояние
				if ((mouse.left < 0) && (mouse.left > -EDIT_ACTION_MOUSE_ZONE))
				{
					point = mClientText->getCursorPoint(mCursorPosition);
					point.left -= (int)EDIT_OFFSET_HORZ_CURSOR;
					action = true;
				}
				// вправо на небольшое расстояние
				else if ((mouse.left > mClient->getWidth()) && (mouse.left < (mClient->getWidth() + EDIT_ACTION_MOUSE_ZONE)))
				{
					point = mClientText->getCursorPoint(mCursorPosition);
					point.left += (int)EDIT_OFFSET_HORZ_CURSOR;
					action = true;
				}

				if (action)
				{
					size_t old = mCursorPosition;
					mCursorPosition = mClientText->getCursorPosition(point);

					if (old != mCursorPosition)
					{
						mClientText->setCursorPosition(mCursorPosition);
						// пытаемся показать курсор
						updateViewWithCursor();
					}
				}
				// если в зону не попадает то сбрасываем
				else
				{
					mActionMouseTimer = 0;
				}

				while (mActionMouseTimer > EDIT_ACTION_MOUSE_TIMER)
					mActionMouseTimer -= EDIT_ACTION_MOUSE_TIMER;
			}

		} // if (mMouseLeftPressed)
	}

	void EditBox::setTextCursor(size_t _index)
	{
		
		// новая позиция
		if (_index > mTextLength)
			_index = mTextLength;

		if (mCursorPosition == _index)
			return;

		mCursorPosition = _index;

		// обновляем по позиции
		if (mClientText != nullptr)
			mClientText->setCursorPosition(mCursorPosition);

        updateViewWithCursor();
	}

	
	void EditBox::setEditPassword(bool _password)
	{
		if (mModePassword == _password)
			return;
		mModePassword = _password;

		if (mModePassword)
		{
			if (mClientText != nullptr)
			{
				mPasswordText = mClientText->getCaption();
				mClientText->setCaption(UString(mTextLength, mCharPassword));
			}
		}
		else
		{
			if (mClientText != nullptr)
			{
				mClientText->setCaption(mPasswordText);
				mPasswordText.clear();
			}
		}
		// обновляем по размерам
		updateView();
	}

	void EditBox::setText(const UString& _caption, bool _history)
	{
	
        UString text = TextIterator::getOnlyText(_caption);
        TextIterator::normaliseNewLine(text,mModeMultiline || mModeWordWrap);
        
		if (mOverflowToTheLeft)
		{
            TextIterator::cutMaxLengthFromBeginning(text,mMaxTextLength);
		}
		else
		{
			// обрезаем по максимальной длинне
			TextIterator::cutMaxLength(text,mMaxTextLength);
		}

		// новая позиция и положение на конец вставки
        mCursorPosition = mTextLength = text.length();

		// и возвращаем строку на место
		setRealString(text);

		// обновляем по позиции
		if (mClientText != nullptr)
			mClientText->setCursorPosition(mCursorPosition);
        updateViewWithCursor();
	}

	void EditBox::insertText(const UString& _text, size_t _start, bool _history)
	{
		// если строка пустая, или размер максимален
		if (_text.empty())
			return;

		if ((mOverflowToTheLeft == false) && (mTextLength == mMaxTextLength))
			return;

		// итератор нашей строки
        UString text = getRealString();
        UString::iterator it = text.begin();
        UString::iterator end = text.end();
        size_t pos = 0;
        // цикл прохода по строке
		while (it!=end)
		{
			// если дошли то выходим
			if (pos == _start)
				break;
            ++it;
            ++pos;
		}

		// а теперь вставляем строку
		text.insert(it,_text);

		if (mOverflowToTheLeft)
		{
            TextIterator::cutMaxLengthFromBeginning(text,mMaxTextLength);
		}
		else
		{
			// обрезаем по максимальной длинне
            TextIterator::cutMaxLength(text,mMaxTextLength);
		}

        size_t old = getRealString().length();
        mTextLength = text.length();
        mCursorPosition += mTextLength - old;

		
		// и возвращаем строку на место
		setRealString(text);

		// обновляем по позиции
		if (mClientText != nullptr)
			mClientText->setCursorPosition(mCursorPosition);
        updateViewWithCursor();
	}

	void EditBox::eraseText(size_t _start, size_t _count, bool _history)
	{
		// чета маловато
		if (_count == 0)
			return;

        // итератор нашей строки
        UString text = getRealString();
        UString::iterator it = text.begin();
        UString::iterator end = text.end();
        UString::iterator start_it = it;
        size_t pos = 0;
        size_t end_pos = _start + _count;
        // цикл прохода по строке
		while (it!=end)
		{
			// сохраняем место откуда начинается
			if (pos == _start)
			{
                start_it = it;
			}

			// окончание диапазона
			else if (pos == end_pos)
			{
				break;
			}
            ++it;
            ++pos;
		}

        text.erase(start_it,it);
		
		// на месте удаленного
		mCursorPosition = _start;
		mTextLength -= _count;

		
		// и возвращаем строку на место
		setRealString(text);

		// обновляем по позиции
		if (mClientText != nullptr)
			mClientText->setCursorPosition(mCursorPosition);
        updateViewWithCursor();
	}

	void EditBox::commandCopy()
	{
        if (!mModePassword) {
            ClipboardManager::getInstance().setClipboardData(EDIT_CLIPBOARD_TYPE_TEXT,getOnlyText());
        } else {
            ClipboardManager::getInstance().clearClipboardData(EDIT_CLIPBOARD_TYPE_TEXT);
        }
	}

	void EditBox::commandPast()
	{
		// копируем из буфера обмена
		std::string clipboard = ClipboardManager::getInstance().getClipboardData(EDIT_CLIPBOARD_TYPE_TEXT);
		if ((!mModeReadOnly) && (!clipboard.empty()))
		{
			insertText(clipboard, mCursorPosition, true);
			// отсылаем событие о изменении
			eventEditTextChange(this);
		}
	}

	const UString& EditBox::getRealString() const
	{
		if (mModePassword)
			return mPasswordText;
		else if (mClientText == nullptr)
			return mPasswordText;

		return mClientText->getCaption();
	}

	void EditBox::setRealString(const UString& _caption)
	{
		if (mModePassword)
		{
			mPasswordText = _caption;
			if (mClientText != nullptr)
				mClientText->setCaption(UString(mTextLength, mCharPassword));
		}
		else
		{
			if (mClientText != nullptr)
				mClientText->setCaption(_caption);
		}
	}
    
    bool EditBox::checkCharInFont(Char _symb) const {
        if (mClientText) {
            return mClientText->fontHasSymbol(_symb);
        }
        return true;
    }

	void EditBox::setPasswordChar(Char _char)
	{
		mCharPassword = _char;
		if (mModePassword)
		{
			if (mClientText != nullptr)
				mClientText->setCaption(UString(mTextLength, mCharPassword));
		}
	}

	void EditBox::updateEditState()
	{
		if (!getInheritedEnabled())
		{
			_setWidgetState("disabled");
		}
		else if (mIsPressed)
		{
			if (mIsFocus)
				_setWidgetState("pushed");
			else
				_setWidgetState("normal_checked");
		}
		else if (mIsFocus)
		{
			_setWidgetState("highlighted");
		}
		else
		{
			_setWidgetState("normal");
		}
	}

	void EditBox::setPosition(const IntPoint& _point)
	{
		Base::setPosition(_point);
	}

	void EditBox::eraseView()
	{
		// если перенос, то сбрасываем размер текста
		if (mModeWordWrap)
		{
			if (mClientText != nullptr)
				mClientText->setWordWrap(true);
		}

		updateView();
	}

	void EditBox::setSize(const IntSize& _size)
	{
		Base::setSize(_size);

		eraseView();
	}

	void EditBox::setCoord(const IntCoord& _coord)
	{
		Base::setCoord(_coord);

		eraseView();
	}

	void EditBox::setCaption(const UString& _value)
	{
		setText(_value, false);
	}

	const UString& EditBox::getCaption() const
	{
		return getRealString();
	}

	
	void EditBox::setTextAlign(Align _value)
	{
		Base::setTextAlign(_value);

		if (mClientText != nullptr)
			mClientText->setTextAlign(_value);

		// так как мы сами рулим смещениями
		updateView();
	}

	void EditBox::setTextColour(const Colour& _value)
	{
		Base::setTextColour(_value);

		if (mClientText != nullptr)
			mClientText->setTextColour(_value);
	}

	IntCoord EditBox::getTextRegion()
	{
		if (mClientText != nullptr)
			return mClientText->getCoord();
		return Base::getTextRegion();
	}

	IntSize EditBox::getTextSize() const
	{
		if (mClientText != nullptr)
			return mClientText->getTextSize();
		return Base::getTextSize();
	}

	void EditBox::notifyScrollChangePosition(ScrollBar* _sender, size_t _position)
	{
		if (mClientText == nullptr)
			return;

		if (_sender == mVScroll)
		{
			IntPoint point = mClientText->getViewOffset();
			point.top = _position;
			mClientText->setViewOffset(point);
		}
		else if (_sender == mHScroll)
		{
			IntPoint point = mClientText->getViewOffset();
			point.left = _position;
			mClientText->setViewOffset(point);
		}
	}

	void EditBox::notifyMouseWheel(Widget* _sender, int _rel)
	{
		if (mClientText == nullptr)
			return;

		if (mVRange != 0)
		{
			IntPoint point = mClientText->getViewOffset();
			int offset = point.top;
			if (_rel < 0)
				offset += EDIT_MOUSE_WHEEL;
			else
				offset -= EDIT_MOUSE_WHEEL;

			if (offset < 0)
				offset = 0;
			else if (offset > (int)mVRange)
				offset = mVRange;

			if (offset != point.top)
			{
				point.top = offset;
				if (mVScroll != nullptr)
					mVScroll->setScrollPosition(offset);
				mClientText->setViewOffset(point);
			}
		}
		else if (mHRange != 0)
		{
			IntPoint point = mClientText->getViewOffset();
			int offset = point.left;
			if (_rel < 0)
				offset += EDIT_MOUSE_WHEEL;
			else
				offset -= EDIT_MOUSE_WHEEL;

			if (offset < 0)
				offset = 0;
			else if (offset > (int)mHRange)
				offset = mHRange;

			if (offset != point.left)
			{
				point.left = offset;
				if (mHScroll != nullptr)
					mHScroll->setScrollPosition(offset);
				mClientText->setViewOffset(point);
			}
		}
	}

	void EditBox::setEditWordWrap(bool _value)
	{
		mModeWordWrap = _value;
		if (mClientText != nullptr)
			mClientText->setWordWrap(mModeWordWrap);

		eraseView();
	}

	void EditBox::setFontName(const std::string& _value)
	{
		Base::setFontName(_value);

		if (mClientText != nullptr)
			mClientText->setFontName(_value);

		eraseView();
	}

	void EditBox::setFontHeight(int _value)
	{
		Base::setFontHeight(_value);

		if (mClientText != nullptr)
			mClientText->setFontHeight(_value);

		eraseView();
	}

	void EditBox::updateView()
	{
		updateScrollSize();
		updateScrollPosition();
	}

	void EditBox::updateViewWithCursor()
	{
		updateScrollSize();
		updateCursorPosition();
		updateScrollPosition();
	}

	void EditBox::updateCursorPosition()
	{
		if (mClientText == nullptr || mClient == nullptr)
			return;

		// размер контекста текста
		IntSize textSize = mClientText->getTextSize();

		// текущее смещение контекста текста
		IntPoint point = mClientText->getViewOffset();
		// расчетное смещение
		IntPoint offset = point;

		// абсолютные координаты курсора
		IntRect cursor = mClientText->getCursorRect(mCursorPosition);
		cursor.right ++;

		// абсолютные координаты вью
		const IntRect& view = mClient->getAbsoluteRect();

		// проверяем и показываем курсор
		if (!view.inside(cursor))
		{
			// горизонтальное смещение
			if (textSize.width > view.width())
			{
				if (cursor.left < view.left)
				{
					offset.left = point.left - (view.left - cursor.left);
					// добавляем смещение, только если курсор не перепрыгнет
					if ((float(view.width()) - EDIT_OFFSET_HORZ_CURSOR) > EDIT_OFFSET_HORZ_CURSOR)
						offset.left -= int(EDIT_OFFSET_HORZ_CURSOR);
				}
				else if (cursor.right > view.right)
				{
					offset.left = point.left + (cursor.right - view.right);
					// добавляем смещение, только если курсор не перепрыгнет
					if ((float(view.width()) - EDIT_OFFSET_HORZ_CURSOR) > EDIT_OFFSET_HORZ_CURSOR)
						offset.left += int(EDIT_OFFSET_HORZ_CURSOR);
				}
			}

			// вертикальное смещение
			if (textSize.height > view.height())
			{
				int delta = 0;
				if (cursor.height() > view.height())
				{
					// if text is bigger than edit height then place it in center
					delta = ((cursor.bottom - view.bottom) - (view.top - cursor.top)) / 2;
				}
				else if (cursor.top < view.top)
				{
					delta = - (view.top - cursor.top);
				}
				else if (cursor.bottom > view.bottom)
				{
					delta = (cursor.bottom - view.bottom);
				}
				offset.top = point.top + delta;
			}

		}

		if (offset != point)
		{
			mClientText->setViewOffset(offset);
			// обновить скролы
			if (mVScroll != nullptr)
				mVScroll->setScrollPosition(offset.top);
			if (mHScroll != nullptr)
				mHScroll->setScrollPosition(offset.left);
		}
	}

	void EditBox::setContentPosition(const IntPoint& _point)
	{
		if (mClientText != nullptr)
			mClientText->setViewOffset(_point);
	}

	IntSize EditBox::getViewSize() const
	{
		if (mClientText != nullptr)
			return mClientText->getSize();
		return ScrollViewBase::getViewSize();
	}

	IntSize EditBox::getContentSize() const
	{
		if (mClientText != nullptr)
			return mClientText->getTextSize();
		return ScrollViewBase::getContentSize();
	}

	size_t EditBox::getVScrollPage()
	{
		if (mClientText != nullptr)
			return (size_t)mClientText->getFontHeight();
		return ScrollViewBase::getVScrollPage();
	}

	size_t EditBox::getHScrollPage()
	{
		if (mClientText != nullptr)
			return (size_t)mClientText->getFontHeight();
		return ScrollViewBase::getHScrollPage();
	}

	IntPoint EditBox::getContentPosition() const
	{
		if (mClientText != nullptr)
			return mClientText->getViewOffset();
		return ScrollViewBase::getContentPosition();
	}

	Align EditBox::getContentAlign()
	{
		if (mClientText != nullptr)
			return mClientText->getTextAlign();
		return ScrollViewBase::getContentAlign();
	}

	void EditBox::setOnlyText(const UString& _text)
	{
		setText(TextIterator::toTagsString(_text), false);
	}

	UString EditBox::getOnlyText()
	{
		return TextIterator::getOnlyText(getRealString());
	}

	void EditBox::insertText(const UString& _text, size_t _index)
	{
		insertText(_text, _index, false);
	}

	void EditBox::addText(const UString& _text)
	{
		insertText(_text, ITEM_NONE, false);
	}

	void EditBox::eraseText(size_t _start, size_t _count)
	{
		eraseText(_start, _count, false);
	}

	void EditBox::setEditReadOnly(bool _value)
	{
		mModeReadOnly = _value;
	}

	void EditBox::setEditMultiLine(bool _value)
	{
		mModeMultiline = _value;
		// на всякий, для убирания переносов
		if (!mModeMultiline)
		{
			setText(getRealString(), false);
		}
		// обновляем по размерам
		else
		{
			updateView();
		}
	}

	void EditBox::setEditStatic(bool _value)
	{
		mModeStatic = _value;
	
		if (mClient != nullptr)
		{
			if (mModeStatic)
				mClient->setPointer("");
			else
				mClient->setPointer(mOriginalPointer);
		}
	}

	void EditBox::setPasswordChar(const UString& _value)
	{
		if (!_value.empty())
			setPasswordChar(*_value.begin());
	}

	void EditBox::setVisibleVScroll(bool _value)
	{
		mVisibleVScroll = _value;
		updateView();
	}

	void EditBox::setVisibleHScroll(bool _value)
	{
		mVisibleHScroll = _value;
		updateView();
	}

	size_t EditBox::getVScrollRange() const
	{
		return mVRange + 1;
	}

	size_t EditBox::getVScrollPosition()
	{
		return mClientText == nullptr ? 0 : mClientText->getViewOffset().top;
	}

	void EditBox::setVScrollPosition(size_t _index)
	{
		if (mClientText == nullptr)
			return;

		if (_index > mVRange)
			_index = mVRange;

		IntPoint point = mClientText->getViewOffset();
		point.top = _index;

		mClientText->setViewOffset(point);
		// обновить скролы
		if (mVScroll != nullptr)
			mVScroll->setScrollPosition(point.top);
	}

	size_t EditBox::getHScrollRange() const
	{
		return mHRange + 1;
	}

	size_t EditBox::getHScrollPosition()
	{
		return mClientText == nullptr ? 0 : mClientText->getViewOffset().left;
	}

	void EditBox::setHScrollPosition(size_t _index)
	{
		if (mClientText == nullptr)
			return;

		if (_index > mHRange)
			_index = mHRange;

		IntPoint point = mClientText->getViewOffset();
		point.left = _index;

		mClientText->setViewOffset(point);
		// обновить скролы
		if (mHScroll != nullptr)
			mHScroll->setScrollPosition(point.left);
	}

	void EditBox::setPropertyOverride(const std::string& _key, const std::string& _value)
	{
		/// @wproperty{EditBox, CursorPosition, size_t} Позиция курсора.
		if (_key == "CursorPosition")
			setTextCursor(utility::parseValue<size_t>(_value));

		/// @wproperty{EditBox, ReadOnly, bool} Режим только для чтения, в этом режиме нельзя изменять текст но которовать можно.
		else if (_key == "ReadOnly")
			setEditReadOnly(utility::parseValue<bool>(_value));

		/// @wproperty{EditBox, Password, bool} Режим ввода пароля, все символы заменяются на звездочки или другие указаные символы.
		else if (_key == "Password")
			setEditPassword(utility::parseValue<bool>(_value));

		/// @wproperty{EditBox, MultiLine, bool} Режим много строчного ввода.
		else if (_key == "MultiLine")
			setEditMultiLine(utility::parseValue<bool>(_value));

		/// @wproperty{EditBox, PasswordChar, string} Символ для замены в режиме пароля.
		else if (_key == "PasswordChar")
			setPasswordChar(_value);

		/// @wproperty{EditBox, MaxTextLength, size_t} Максимальное длина текста.
		else if (_key == "MaxTextLength")
			setMaxTextLength(utility::parseValue<size_t>(_value));

		/// @wproperty{EditBox, OverflowToTheLeft, bool} Режим обрезки текста в начале, после того как его колличество достигает максимального значения.
		else if (_key == "OverflowToTheLeft")
			setOverflowToTheLeft(utility::parseValue<bool>(_value));

		/// @wproperty{EditBox, Static, bool} Статический режим, поле ввода никак не реагирует на пользовательский ввод.
		else if (_key == "Static")
			setEditStatic(utility::parseValue<bool>(_value));

		/// @wproperty{EditBox, VisibleVScroll, bool} Видимость вертикальной полосы прокрутки.
		else if (_key == "VisibleVScroll")
			setVisibleVScroll(utility::parseValue<bool>(_value));

		/// @wproperty{EditBox, VisibleHScroll, bool} Видимость горизонтальной полосы прокрутки.
		else if (_key == "VisibleHScroll")
			setVisibleHScroll(utility::parseValue<bool>(_value));

		/// @wproperty{EditBox, WordWrap, bool} Режим переноса по словам.
		else if (_key == "WordWrap")
			setEditWordWrap(utility::parseValue<bool>(_value));

		/// @wproperty{EditBox, TabPrinting, bool} Воспринимать нажатие на Tab как символ табуляции.
		else if (_key == "TabPrinting")
			setTabPrinting(utility::parseValue<bool>(_value));

        else
		{
			Base::setPropertyOverride(_key, _value);
			return;
		}

		eventChangeProperty(this, _key, _value);
	}

	size_t EditBox::getTextCursor() const
	{
		return mCursorPosition;
	}

	size_t EditBox::getTextLength() const
	{
		return mTextLength;
	}

	void EditBox::setOverflowToTheLeft(bool _value)
	{
		mOverflowToTheLeft = _value;
	}

	bool EditBox::getOverflowToTheLeft() const
	{
		return mOverflowToTheLeft;
	}

	void EditBox::setMaxTextLength(size_t _value)
	{
		mMaxTextLength = _value;
	}

	size_t EditBox::getMaxTextLength() const
	{
		return mMaxTextLength;
	}

	bool EditBox::getEditReadOnly() const
	{
		return mModeReadOnly;
	}

	bool EditBox::getEditPassword() const
	{
		return mModePassword;
	}

	bool EditBox::getEditMultiLine() const
	{
		return mModeMultiline;
	}

	bool EditBox::getEditStatic() const
	{
		return mModeStatic;
	}

	Char EditBox::getPasswordChar() const
	{
		return mCharPassword;
	}

	bool EditBox::getEditWordWrap() const
	{
		return mModeWordWrap;
	}

	void EditBox::setTabPrinting(bool _value)
	{
		mTabPrinting = _value;
	}

	bool EditBox::getTabPrinting() const
	{
		return mTabPrinting;
	}

	void EditBox::setPosition(int _left, int _top)
	{
		setPosition(IntPoint(_left, _top));
	}

	void EditBox::setSize(int _width, int _height)
	{
		setSize(IntSize(_width, _height));
	}

	void EditBox::setCoord(int _left, int _top, int _width, int _height)
	{
		setCoord(IntCoord(_left, _top, _width, _height));
	}

	bool EditBox::isVisibleVScroll() const
	{
		return mVisibleVScroll;
	}

	bool EditBox::isVisibleHScroll() const
	{
		return mVisibleHScroll;
	}

    void EditBox::setTextPassColour(const std::string& pass,const Colour& _value)
	{
		Base::setTextPassColour(pass,_value);

		if (mClientText != nullptr)
			mClientText->setPassColour(pass,_value);
	}

} // namespace MyGUI
