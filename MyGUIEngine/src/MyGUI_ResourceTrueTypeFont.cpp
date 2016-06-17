/*
 * This source file is part of MyGUI. For the latest info, see http://mygui.info/
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#include "MyGUI_Precompiled.h"
#include "MyGUI_ResourceTrueTypeFont.h"
#include "MyGUI_DataManager.h"
#include "MyGUI_DataStreamHolder.h"
#include "MyGUI_RenderManager.h"
#include "MyGUI_Bitwise.h"

#ifdef MYGUI_USE_FREETYPE

#	include FT_GLYPH_H
#	include FT_TRUETYPE_TABLES_H
#	include FT_BITMAP_H
#	include FT_WINFONTS_H
#   include FT_STROKER_H



namespace MyGUI
{


	namespace
	{

		template<typename T>
		void setMax(T& _var, const T& _newValue)
		{
			if (_var < _newValue)
				_var = _newValue;
		}

		std::pair<const Char, const uint8> charMaskData[] =
		{
			std::make_pair(FontCodeType::Selected, (const uint8)'\x88'),
			std::make_pair(FontCodeType::SelectedBack, (const uint8)'\x60'),
			std::make_pair(FontCodeType::Cursor, (const uint8)'\xFF'),
			std::make_pair(FontCodeType::Tab, (const uint8)'\x00')
		};

		const std::map<const Char, const uint8> charMask(charMaskData, charMaskData + sizeof charMaskData / sizeof(*charMaskData));

		const uint8 charMaskBlack = (const uint8)'\x00';
		const uint8 charMaskWhite = (const uint8)'\xFF';

		
		struct PixelBase
		{
			static size_t getNumBytes()
			{
				return 4;
			}

			static PixelFormat::Enum getFormat()
			{
				return PixelFormat::R8G8B8A8;
			}

			static void set(uint8*& _dest, uint8 _luminance, uint8 _alpha)
			{
#ifdef MYGUI_USE_PREMULTIPLIED_ALPHA
                _luminance = (uint16(_luminance) * _alpha) >> 8;
#endif
                *_dest++ = _luminance;
                *_dest++ = _luminance;
                *_dest++ = _luminance;
                *_dest++ = _alpha;
			}
            
            static void set(uint8*& _dest, const Colour& _luminance, uint8 _alpha)
            {
#ifdef MYGUI_USE_PREMULTIPLIED_ALPHA
                *_dest++ = _luminance.red * _alpha ;
                *_dest++ = _luminance.green * _alpha ;
                *_dest++ = _luminance.blue * _alpha;
                *_dest++ = _alpha;
#else
                *_dest++ = _luminance.red * 255;
                *_dest++ = _luminance.green * 255;
                *_dest++ = _luminance.blue * 255;
                *_dest++ = _alpha;
#endif
            }
            
            static void blend_c(uint8& _dest, float _src, uint8 _a) {
#ifdef MYGUI_USE_PREMULTIPLIED_ALPHA
                _dest = (uint16(_dest) * (255-_a))/255 + (_src * _a);
#endif
            }
            
            static void blend(uint8*& _dest, const Colour& _clr, uint8 _alpha)
            {
                blend_c(*_dest++ , _clr.red , _alpha);
                blend_c(*_dest++ , _clr.green , _alpha);
                blend_c(*_dest++ , _clr.blue , _alpha);
                blend_c(*_dest++ , _alpha / 255.0f , _alpha);

            }
		};

		
	}

	const int ResourceTrueTypeFont::mDefaultGlyphSpacing = 1;
	const float ResourceTrueTypeFont::mDefaultTabWidth = 8.0f;
	const float ResourceTrueTypeFont::mSelectedWidth = 1.0f;
	const float ResourceTrueTypeFont::mCursorWidth = 2.0f;

	ResourceTrueTypeFont::ResourceTrueTypeFont() :
		mSize(0),
        mScale(1.0f),
		mResolution(96),
		mHinting(HintingUseNative),
	    mOutline(false),
        mOutlineColour(0,0,0,1),
        mOutlineWidth(1.0f),
		mSpaceWidth(0.0f),
		mGlyphSpacing(-1),
		mTabWidth(0.0f),
		mOffsetHeight(0),
		mSubstituteCodePoint(static_cast<Char>(FontCodeType::NotDefined)),
		mDefaultHeight(0),
		mSubstituteGlyphInfo(nullptr),
		mTexture(nullptr)
	{
	}

	ResourceTrueTypeFont::~ResourceTrueTypeFont()
	{
		if (mTexture != nullptr)
		{
			RenderManager::getInstance().destroyTexture(mTexture);
			mTexture = nullptr;
		}
	}

	void ResourceTrueTypeFont::deserialization(xml::ElementPtr _node, Version _version)
	{
		Base::deserialization(_node, _version);

		xml::ElementEnumerator node = _node->getElementEnumerator();
		while (node.next())
		{
			if (node->getName() == "Property")
			{
				const std::string& key = node->findAttribute("key");
				const std::string& value = node->findAttribute("value");
				if (key == "Source")
					setSource(value);
				else if (key == "Size")
					setSize(utility::parseFloat(value));
                else if (key == "Scale")
                    setScale(utility::parseFloat(value));
				else if (key == "Resolution")
					setResolution(utility::parseUInt(value));
				else if (key == "Antialias")
                {}
                else if (key == "Outline")
                    setOutline(utility::parseBool(value));
                else if (key == "OutlineColour")
                    setOutlineColour(utility::parseValue<Colour>(value));
                else if (key == "OutlineWidth")
                    setOutlineWidth(utility::parseFloat(value));
				else if (key == "TabWidth")
					setTabWidth(utility::parseFloat(value));
				else if (key == "OffsetHeight")
					setOffsetHeight(utility::parseInt(value));
				else if (key == "SubstituteCode")
					setSubstituteCode(utility::parseInt(value));
				else if (key == "Distance")
					setDistance(utility::parseInt(value));
				else if (key == "Hinting")
					setHinting(value);
				else if (key == "SpaceWidth")
				{
					mSpaceWidth = utility::parseFloat(value);
					MYGUI_LOG(Warning, _node->findAttribute("type") << ": Property '" << key << "' in font '" << _node->findAttribute("name") << "' is deprecated; remove it to use automatic calculation.");
				}
				else if (key == "CursorWidth")
				{
					MYGUI_LOG(Warning, _node->findAttribute("type") << ": Property '" << key << "' in font '" << _node->findAttribute("name") << "' is deprecated; value ignored.");
				}
			}
			else if (node->getName() == "Codes")
			{
				// Range of inclusions.
				xml::ElementEnumerator range = node->getElementEnumerator();
				while (range.next("Code"))
				{
					std::string range_value;
					if (range->findAttribute("range", range_value))
					{
						std::vector<std::string> parse_range = utility::split(range_value);
						if (!parse_range.empty())
						{
							Char first = utility::parseUInt(parse_range[0]);
							Char last = parse_range.size() > 1 ? utility::parseUInt(parse_range[1]) : first;
							addCodePointRange(first, last);
						}
					}
				}

				// If no code points have been included, include the Unicode Basic Multilingual Plane by default before processing
				//	any exclusions.
				if (mCharMap.empty())
					addCodePointRange(0, 0xFFFF);

				// Range of exclusions.
				range = node->getElementEnumerator();
				while (range.next("Code"))
				{
					std::string range_value;
					if (range->findAttribute("hide", range_value))
					{
						std::vector<std::string> parse_range = utility::split(range_value);
						if (!parse_range.empty())
						{
							Char first = utility::parseUInt(parse_range[0]);
							Char last = parse_range.size() > 1 ? utility::parseUInt(parse_range[1]) : first;
							removeCodePointRange(first, last);
						}
					}
				}
			}
		}

		initialise();
	}

	GlyphInfo* ResourceTrueTypeFont::getGlyphInfo(Char _id)
	{
		CharMap::const_iterator charIter = mCharMap.find(_id);

		if (charIter != mCharMap.end())
		{
			GlyphMap::iterator glyphIter = mGlyphMap.find(charIter->second);

			if (glyphIter != mGlyphMap.end())
				return &glyphIter->second;
		}

		return mSubstituteGlyphInfo;
	}

	int ResourceTrueTypeFont::getDefaultHeight()
	{
		return mDefaultHeight;
	}

	void ResourceTrueTypeFont::textureInvalidate(ITexture* _texture)
	{
		mGlyphMap.clear();
		initialise();
	}

	std::vector<std::pair<Char, Char> > ResourceTrueTypeFont::getCodePointRanges() const
	{
		std::vector<std::pair<Char, Char> > result;

		if (!mCharMap.empty())
		{
			CharMap::const_iterator iter = mCharMap.begin(), endIter = mCharMap.end();

			// Start the first range with the first code point in the map.
			Char rangeBegin = iter->first, rangeEnd = rangeBegin;

			// Loop over the rest of the map and find the contiguous ranges.
			for (++iter; iter != endIter; ++iter)
			{
				if (iter->first == rangeEnd + 1)
				{
					// Extend the current range.
					++rangeEnd;
				}
				else
				{
					// Found the start of a new range. First, save the current range.
					result.push_back(std::make_pair(rangeBegin, rangeEnd));

					// Then start the new range.
					rangeBegin = rangeEnd = iter->first;
				}
			}

			// Save the final range.
			result.push_back(std::make_pair(rangeBegin, rangeEnd));
		}

		return result;
	}

	Char ResourceTrueTypeFont::getSubstituteCodePoint() const
	{
		return mSubstituteCodePoint;
	}

	void ResourceTrueTypeFont::addCodePoint(Char _codePoint)
	{
		mCharMap.insert(CharMap::value_type(_codePoint, 0));
	}

	void ResourceTrueTypeFont::removeCodePoint(Char _codePoint)
	{
		mCharMap.erase(_codePoint);
	}

	void ResourceTrueTypeFont::addCodePointRange(Char _first, Char _second)
	{
		CharMap::iterator positionHint = mCharMap.lower_bound(_first);

		if (positionHint != mCharMap.begin())
			--positionHint;

		for (Char i = _first; i <= _second; ++i)
			positionHint = mCharMap.insert(positionHint, CharMap::value_type(i, 0));
	}

	void ResourceTrueTypeFont::removeCodePointRange(Char _first, Char _second)
	{
		mCharMap.erase(mCharMap.lower_bound(_first), mCharMap.upper_bound(_second));
	}

	void ResourceTrueTypeFont::clearCodePoints()
	{
		mCharMap.clear();
	}

	void ResourceTrueTypeFont::initialise()
	{
		if (mGlyphSpacing == -1)
			mGlyphSpacing = mDefaultGlyphSpacing;
        
        mScale = MyGUI::RenderManager::getInstance().getDisplayScale();

		
		// Select and call an appropriate initialisation method. By making this decision up front, we avoid having to branch on
		// these variables many thousands of times inside tight nested loops later. From this point on, the various function
		// templates ensure that all of the necessary branching is done purely at compile time for all combinations.
        int init = (mOutline ? 1 : 0);

		switch (init)
		{
		case 0:
			ResourceTrueTypeFont::initialiseFreeType<false>();
			break;
		case 1:
			ResourceTrueTypeFont::initialiseFreeType<true>();
			break;
		}
	}

	template<bool Outline>
	void ResourceTrueTypeFont::initialiseFreeType()
	{
		//-------------------------------------------------------------------//
		// Initialise FreeType and load the font.
		//-------------------------------------------------------------------//

		FT_Library ftLibrary;

		if (FT_Init_FreeType(&ftLibrary) != 0)
			MYGUI_EXCEPT("ResourceTrueTypeFont: Could not init the FreeType library!");

		uint8* fontBuffer = nullptr;

		FT_Face ftFace = loadFace(ftLibrary, fontBuffer);

		if (ftFace == nullptr)
		{
			MYGUI_LOG(Error, "ResourceTrueTypeFont: Could not load the font '" << getResourceName() << "'!");
			return;
		}

		//-------------------------------------------------------------------//
		// Calculate the font metrics.
		//-------------------------------------------------------------------//

		// The font's overall ascent and descent are defined in three different places in a TrueType font, and with different
		// values in each place. The most reliable source for these metrics is usually the "usWinAscent" and "usWinDescent" pair of
		// values in the OS/2 header; however, some fonts contain inaccurate data there. To be safe, we use the highest of the set
		// of values contained in the face metrics and the two sets of values contained in the OS/2 header.
		int fontAscent = ftFace->size->metrics.ascender >> 6;
		int fontDescent = -ftFace->size->metrics.descender >> 6;

		TT_OS2* os2 = (TT_OS2*)FT_Get_Sfnt_Table(ftFace, ft_sfnt_os2);

		if (os2 != nullptr)
		{
			setMax(fontAscent, os2->usWinAscent * ftFace->size->metrics.y_ppem / ftFace->units_per_EM);
			setMax(fontDescent, os2->usWinDescent * ftFace->size->metrics.y_ppem / ftFace->units_per_EM);

			setMax(fontAscent, os2->sTypoAscender * ftFace->size->metrics.y_ppem / ftFace->units_per_EM);
			setMax(fontDescent, -os2->sTypoDescender * ftFace->size->metrics.y_ppem / ftFace->units_per_EM);
		}

		// The nominal font height is calculated as the sum of its ascent and descent as specified by the font designer. Previously
		// it was defined by MyGUI in terms of the maximum ascent and descent of the glyphs currently in use, but this caused the
		// font's line spacing to change whenever glyphs were added to or removed from the font definition. Doing it this way
		// instead prevents a lot of layout problems, and it is also more typographically correct and more aesthetically pleasing.
		mDefaultHeight = fontAscent + fontDescent;

		// Set the load flags based on the specified type of hinting.
		FT_Int32 ftLoadFlags;

		switch (mHinting)
		{
		case HintingForceAuto:
			ftLoadFlags = FT_LOAD_FORCE_AUTOHINT;
			break;
		case HintingDisableAuto:
			ftLoadFlags = FT_LOAD_NO_AUTOHINT;
			break;
		case HintingDisableAll:
			// When hinting is completely disabled, glyphs must always be rendered -- even during layout calculations -- due to
			// discrepancies between the glyph metrics and the actual rendered bitmap metrics.
			ftLoadFlags = FT_LOAD_NO_HINTING | FT_LOAD_RENDER;
			break;
		//case HintingUseNative:
		default:
			ftLoadFlags = FT_LOAD_DEFAULT;
			break;
		}

		//-------------------------------------------------------------------//
		// Create the glyphs and calculate their metrics.
		//-------------------------------------------------------------------//

		GlyphHeightMap glyphHeightMap;
		int texWidth = 0;

		// Before creating the glyphs, add the "Space" code point to force that glyph to be created. For historical reasons, MyGUI
		// users are accustomed to omitting this code point in their font definitions.
		addCodePoint(FontCodeType::Space);

		// Create the standard glyphs.
		for (CharMap::iterator iter = mCharMap.begin(); iter != mCharMap.end(); )
		{
			const Char& codePoint = iter->first;
			FT_UInt glyphIndex = FT_Get_Char_Index(ftFace, codePoint);

			texWidth += createFaceGlyph(ftLibrary,glyphIndex, codePoint, fontAscent, ftFace, ftLoadFlags, glyphHeightMap);

			// If the newly created glyph is the "Not Defined" glyph, it means that the code point is not supported by the font.
			// Remove it from the character map so that we can provide our own substitute instead of letting FreeType do it.
			if (iter->second != 0)
				++iter;
			else
				mCharMap.erase(iter++);
		}


		// Do some special handling for the "Space" and "Tab" glyphs.
		GlyphInfo* spaceGlyphInfo = getGlyphInfo(FontCodeType::Space);

		if (spaceGlyphInfo != nullptr && spaceGlyphInfo->codePoint == FontCodeType::Space)
		{
			// Adjust the width of the "Space" glyph if it has been customized.
			if (mSpaceWidth != 0.0f)
			{
				texWidth += (int)ceil(mSpaceWidth) - (int)ceil(spaceGlyphInfo->width);
				spaceGlyphInfo->width = mSpaceWidth;
				spaceGlyphInfo->advance = mSpaceWidth;
			}

			// If the width of the "Tab" glyph hasn't been customized, make it eight spaces wide.
			if (mTabWidth == 0.0f)
				mTabWidth = mDefaultTabWidth * spaceGlyphInfo->advance;
		}

		// Create the special glyphs. They must be created after the standard glyphs so that they take precedence in case of a
		// collision. To make sure that the indices of the special glyphs don't collide with any glyph indices in the font, we must
		// use glyph indices higher than the highest glyph index in the font.
		FT_UInt nextGlyphIndex = (FT_UInt)ftFace->num_glyphs;

		float height = (float)mDefaultHeight;

		texWidth += createGlyph(nextGlyphIndex++, GlyphInfo(static_cast<Char>(FontCodeType::Tab), 0.0f, 0.0f, mTabWidth, 0.0f, 0.0f), glyphHeightMap);
		texWidth += createGlyph(nextGlyphIndex++, GlyphInfo(static_cast<Char>(FontCodeType::Selected), mSelectedWidth, height, 0.0f, 0.0f, 0.0f), glyphHeightMap);
		texWidth += createGlyph(nextGlyphIndex++, GlyphInfo(static_cast<Char>(FontCodeType::SelectedBack), mSelectedWidth, height, 0.0f, 0.0f, 0.0f), glyphHeightMap);
		texWidth += createGlyph(nextGlyphIndex++, GlyphInfo(static_cast<Char>(FontCodeType::Cursor), mCursorWidth, height, 0.0f, 0.0f, 0.0f), glyphHeightMap);

		// If a substitute code point has been specified, check to make sure that it exists in the character map. If it doesn't,
		// revert to the default "Not Defined" code point. This is not a real code point but rather an invalid Unicode value that
		// is guaranteed to cause the "Not Defined" special glyph to be created.
		if (mSubstituteCodePoint != FontCodeType::NotDefined && mCharMap.find(mSubstituteCodePoint) == mCharMap.end())
			mSubstituteCodePoint = static_cast<Char>(FontCodeType::NotDefined);

		// Create the "Not Defined" code point (and its corresponding glyph) if it's in use as the substitute code point.
		if (mSubstituteCodePoint == FontCodeType::NotDefined)
			texWidth += createFaceGlyph(ftLibrary, 0, static_cast<Char>(FontCodeType::NotDefined), fontAscent, ftFace, ftLoadFlags, glyphHeightMap);

		// Cache a pointer to the substitute glyph info for fast lookup.
		mSubstituteGlyphInfo = &mGlyphMap.find(mCharMap.find(mSubstituteCodePoint)->second)->second;

		// Calculate the average height of all of the glyphs that are in use. This value will be used for estimating how large the
		// texture needs to be.
		double averageGlyphHeight = 0.0;

		for (GlyphHeightMap::const_iterator j = glyphHeightMap.begin(); j != glyphHeightMap.end(); ++j)
			averageGlyphHeight += j->first * j->second.size();

		averageGlyphHeight /= mGlyphMap.size();

		//-------------------------------------------------------------------//
		// Calculate the final texture size.
		//-------------------------------------------------------------------//

		// Round the current texture width and height up to the nearest powers of two.
		texWidth = Bitwise::firstPO2From(texWidth);
		int texHeight = Bitwise::firstPO2From((int)ceil(averageGlyphHeight) + mGlyphSpacing);

		// At this point, the texture is only one glyph high and is probably very wide. For efficiency reasons, we need to make the
		// texture as square as possible. If the texture cannot be made perfectly square, make it taller than it is wide, because
		// the height may decrease in the final layout due to height packing.
		while (texWidth > texHeight)
		{
			texWidth /= 2;
			texHeight *= 2;
		}

		// Calculate the final layout of all of the glyphs in the texture, packing them tightly by first arranging them by height.
		// We assume that the texture width is fixed but that the texture height can be adjusted up or down depending on how much
		// space is actually needed.
		// In most cases, the final texture will end up square or almost square. In some rare cases, however, we can end up with a
		// texture that's more than twice as high as it is wide; when this happens, we double the width and try again.
		do
		{
			if (texHeight > texWidth * 2)
				texWidth *= 2;

			int texX = 0, texY = 0;

			for (GlyphHeightMap::const_iterator j = glyphHeightMap.begin(); j != glyphHeightMap.end(); ++j)
			{
				for (GlyphHeightMap::mapped_type::const_iterator i = j->second.begin(); i != j->second.end(); ++i)
				{
					GlyphInfo& info = *i->second;

					int glyphWidth = (int)ceil(info.width);
					int glyphHeight = (int)ceil(info.height);

					autoWrapGlyphPos(glyphWidth, texWidth, glyphHeight, texX, texY);

					if (glyphWidth > 0)
						texX += mGlyphSpacing + glyphWidth;
				}
			}

			texHeight = Bitwise::firstPO2From(texY + glyphHeightMap.rbegin()->first);
		}
		while (texHeight > texWidth * 2);

		//-------------------------------------------------------------------//
		// Create the texture and render the glyphs onto it.
		//-------------------------------------------------------------------//

		if (mTexture)
		{
			RenderManager::getInstance().destroyTexture( mTexture );
			mTexture = nullptr;
		}

		mTexture = RenderManager::getInstance().createTexture(MyGUI::utility::toString((size_t)this, "_TrueTypeFont"));

		mTexture->createManual(texWidth, texHeight, TextureUsage::Static | TextureUsage::Write, PixelBase::getFormat());
		mTexture->setInvalidateListener(this);

		uint8* texBuffer = static_cast<uint8*>(mTexture->lock(TextureUsage::Write));

		if (texBuffer != nullptr)
		{
			// Make the texture background transparent white.
            ::memset(texBuffer, 0, texWidth*texHeight*PixelBase::getNumBytes());

			renderGlyphs<Outline>(glyphHeightMap, ftLibrary, ftFace, ftLoadFlags, texBuffer, texWidth, texHeight);

			mTexture->unlock();

			MYGUI_LOG(Info, "ResourceTrueTypeFont: Font '" << getResourceName() << "' using texture size " << texWidth << " x " << texHeight << ".");
			MYGUI_LOG(Info, "ResourceTrueTypeFont: Font '" << getResourceName() << "' using real height " << mDefaultHeight << " pixels.");
		}
		else
		{
			MYGUI_LOG(Error, "ResourceTrueTypeFont: Error locking texture; pointer is nullptr.");
		}
        
        for (std::map<FT_UInt, FT_Bitmap>::iterator it = m_bitmaps_map.begin();it!=m_bitmaps_map.end();++it) {
            FT_Bitmap_Done(ftLibrary, &it->second);
        }
        for (std::map<FT_UInt, FT_Glyph>::iterator it = m_outline_bitmaps_map.begin();it!=m_outline_bitmaps_map.end();++it) {
            FT_Done_Glyph(it->second);
        }

		FT_Done_Face(ftFace);
		FT_Done_FreeType(ftLibrary);
        
        if (mScale != 1.0f) {
            float iscale = 1.0f / mScale;
            for (GlyphMap::iterator iter = mGlyphMap.begin(); iter != mGlyphMap.end(); ++iter)
            {
                iter->second.width *= iscale;
                iter->second.height *= iscale;
                iter->second.advance *= iscale;
                iter->second.bearingX *= iscale;
                iter->second.bearingY *= iscale;
                iter->second.texture = mTexture;
            }
            mSpaceWidth *= iscale;
            mOffsetHeight *= iscale;
            mGlyphSpacing *= iscale;
            mDefaultHeight *= iscale;
        } else {
            for (GlyphMap::iterator iter = mGlyphMap.begin(); iter != mGlyphMap.end(); ++iter)
            {
                iter->second.texture = mTexture;
            }
        }

		delete [] fontBuffer;
	}

	FT_Face ResourceTrueTypeFont::loadFace(const FT_Library& _ftLibrary, uint8*& _fontBuffer)
	{
		FT_Face result = nullptr;

		// Load the font file.
		IDataStream* datastream = DataManager::getInstance().getData(mSource);

		if (datastream == nullptr)
			return result;

		size_t fontBufferSize = datastream->size();
		_fontBuffer = new uint8[fontBufferSize];
		datastream->read(_fontBuffer, fontBufferSize);

		DataManager::getInstance().freeData(datastream);
		datastream = nullptr;

		// Determine how many faces the font contains.
		if (FT_New_Memory_Face(_ftLibrary, _fontBuffer, (FT_Long)fontBufferSize, -1, &result) != 0)
			MYGUI_EXCEPT("ResourceTrueTypeFont: Could not load the font '" << getResourceName() << "'!");

		FT_Long numFaces = result->num_faces;
		FT_Long faceIndex = 0;

		// Load the first face.
		if (FT_New_Memory_Face(_ftLibrary, _fontBuffer, (FT_Long)fontBufferSize, faceIndex, &result) != 0)
			MYGUI_EXCEPT("ResourceTrueTypeFont: Could not load the font '" << getResourceName() << "'!");

		if (result->face_flags & FT_FACE_FLAG_SCALABLE)
		{
			// The font is scalable, so set the font size by first converting the requested size to FreeType's 26.6 fixed-point
			// format.
			FT_F26Dot6 ftSize = (FT_F26Dot6)((mSize*mScale) * (1 << 6));

			if (FT_Set_Char_Size(result, ftSize, 0, mResolution, mResolution) != 0)
				MYGUI_EXCEPT("ResourceTrueTypeFont: Could not set the font size for '" << getResourceName() << "'!");

			// If no code points have been specified, use the Unicode Basic Multilingual Plane by default.
			if (mCharMap.empty())
				addCodePointRange(0, 0xFFFF);
		}
		else
		{
			MYGUI_EXCEPT("ResourceTrueTypeFont: Could not load the font '" << getResourceName() << "'!");
		}

		return result;
	}

	void ResourceTrueTypeFont::autoWrapGlyphPos(int _glyphWidth, int _texWidth, int _lineHeight, int& _texX, int& _texY)
	{
		if (_glyphWidth > 0 && _texX + mGlyphSpacing + _glyphWidth > _texWidth)
		{
			_texX = 0;
			_texY += mGlyphSpacing + _lineHeight;
		}
	}

	GlyphInfo ResourceTrueTypeFont::createFaceGlyphInfo(Char _codePoint, int _fontAscent, FT_GlyphSlot _glyph)
	{
		float bearingX = _glyph->metrics.horiBearingX / 64.0f;

		// The following calculations aren't currently needed but are kept here for future use.
		// float ascent = _glyph->metrics.horiBearingY / 64.0f;
		// float descent = (_glyph->metrics.height / 64.0f) - ascent;

		return GlyphInfo(
			_codePoint,
			std::max((float)_glyph->bitmap.width, _glyph->metrics.width / 64.0f),
			std::max((float)_glyph->bitmap.rows, _glyph->metrics.height / 64.0f),
			(_glyph->advance.x / 64.0f) - bearingX,
			bearingX,
			floor(_fontAscent - (_glyph->metrics.horiBearingY / 64.0f) - mOffsetHeight));
	}

	int ResourceTrueTypeFont::createGlyph(FT_UInt _glyphIndex, const GlyphInfo& _glyphInfo, GlyphHeightMap& _glyphHeightMap)
	{
		int width = (int)ceil(_glyphInfo.width);
		int height = (int)ceil(_glyphInfo.height);

		mCharMap[_glyphInfo.codePoint] = _glyphIndex;
		GlyphInfo& info = mGlyphMap.insert(GlyphMap::value_type(_glyphIndex, _glyphInfo)).first->second;
		_glyphHeightMap[(FT_Pos)height].insert(std::make_pair(_glyphIndex, &info));
        
		return (width > 0) ? mGlyphSpacing + width : 0;
	}

	int ResourceTrueTypeFont::createFaceGlyph(const FT_Library& _ftLibrary,FT_UInt _glyphIndex, Char _codePoint, int _fontAscent, const FT_Face& _ftFace, FT_Int32 _ftLoadFlags, GlyphHeightMap& _glyphHeightMap)
	{
		if (mGlyphMap.find(_glyphIndex) == mGlyphMap.end())
		{
            if (mOutline) {
                
                if (FT_Load_Glyph(_ftFace, _glyphIndex, (_ftLoadFlags & (~FT_LOAD_RENDER)) | FT_LOAD_NO_BITMAP) == 0) {
                    // Set up a stroker.
                    int res = 0;
                    float outlineWidth = mOutlineWidth * mScale;
                    FT_Stroker stroker;
                    FT_Stroker_New(_ftLibrary, &stroker);
                    FT_Stroker_Set(stroker,
                                   (int)(outlineWidth * 64),
                                   FT_STROKER_LINECAP_ROUND,
                                   FT_STROKER_LINEJOIN_ROUND,
                                   0);
                    
                    FT_Glyph glyph;
                    if (FT_Get_Glyph(_ftFace->glyph, &glyph) == 0)
                    {
                        FT_Glyph_StrokeBorder(&glyph, stroker,0, 1);
                        // Render the outline spans to the span list
                        FT_Glyph_To_Bitmap(&glyph,FT_RENDER_MODE_NORMAL,0,1);
                        FT_BitmapGlyph ft_bitmap_glyph = (FT_BitmapGlyph) glyph;
                     
                        m_outline_bitmaps_map[_glyphIndex] = glyph;
                        
                        float bearingX = _ftFace->glyph->metrics.horiBearingX / 64.0f;
                        
                        // The following calculations aren't currently needed but are kept here for future use.
                        // float ascent = _glyph->metrics.horiBearingY / 64.0f;
                        // float descent = (_glyph->metrics.height / 64.0f) - ascent;
                        
                        GlyphInfo info(
                                         _codePoint,
                                         ft_bitmap_glyph->bitmap.width,
                                         ft_bitmap_glyph->bitmap.rows,
                                         (_ftFace->glyph->advance.x / 64.0f) - bearingX,
                                         bearingX,
                                         floor(_fontAscent - (_ftFace->glyph->metrics.horiBearingY / 64.0f) - mOffsetHeight));
                        
                        res = createGlyph(_glyphIndex, info, _glyphHeightMap);
                        
                        if (FT_Load_Glyph(_ftFace, _glyphIndex, _ftLoadFlags | FT_LOAD_RENDER) == 0) {
                            FT_Bitmap new_bitmap;
                            FT_Bitmap_New(&new_bitmap);
                            FT_Bitmap_Copy(_ftLibrary, &_ftFace->glyph->bitmap, &new_bitmap);
                            m_bitmaps_map[_glyphIndex] = new_bitmap;
                            
                            ft_bitmap_glyph->left -= _ftFace->glyph->bitmap_left;
                            ft_bitmap_glyph->top -= _ftFace->glyph->bitmap_top;
                        }

                    }
                    // Clean up afterwards.
                    FT_Stroker_Done(stroker);
                    
                    
                    
                    return res;
                } else {
                    MYGUI_LOG(Warning, "ResourceTrueTypeFont: Cannot load glyph " << _glyphIndex << " for character " << _codePoint << " in font '" << getResourceName() << "'.");
                }

            } else {
                if (FT_Load_Glyph(_ftFace, _glyphIndex, _ftLoadFlags | FT_LOAD_RENDER) == 0) {
                    FT_Bitmap new_bitmap;
                    FT_Bitmap_New(&new_bitmap);
                    FT_Bitmap_Copy(_ftLibrary, &_ftFace->glyph->bitmap, &new_bitmap);
                    m_bitmaps_map[_glyphIndex] = new_bitmap;
                    return createGlyph(_glyphIndex, createFaceGlyphInfo(_codePoint, _fontAscent, _ftFace->glyph), _glyphHeightMap);
                }
                else
                    MYGUI_LOG(Warning, "ResourceTrueTypeFont: Cannot load glyph " << _glyphIndex << " for character " << _codePoint << " in font '" << getResourceName() << "'.");
            }
			
		}
		else
		{
			mCharMap[_codePoint] = _glyphIndex;
		}

		return 0;
	}

	template<bool Outline>
	void ResourceTrueTypeFont::renderGlyphs(const GlyphHeightMap& _glyphHeightMap, const FT_Library& _ftLibrary, const FT_Face& _ftFace, FT_Int32 _ftLoadFlags, uint8* _texBuffer, int _texWidth, int _texHeight)
	{
		FT_Bitmap ftBitmap;
		FT_Bitmap_New(&ftBitmap);

		int texX = 0, texY = 0;

		for (GlyphHeightMap::const_iterator j = _glyphHeightMap.begin(); j != _glyphHeightMap.end(); ++j)
		{
			for (GlyphHeightMap::mapped_type::const_iterator i = j->second.begin(); i != j->second.end(); ++i)
			{
				GlyphInfo& info = *i->second;

				switch (info.codePoint)
				{
				case FontCodeType::Selected:
				case FontCodeType::SelectedBack:
				{
					fillGlyph(info, charMaskWhite, charMask.find(info.codePoint)->second, j->first, _texBuffer, _texWidth, _texHeight, texX, texY);

					// Manually adjust the glyph's width to zero. This prevents artifacts from appearing at the seams when
					// rendering multi-character selections.
					GlyphInfo* glyphInfo = getGlyphInfo(info.codePoint);
					glyphInfo->width = 0.0f;
					glyphInfo->uvRect.right = glyphInfo->uvRect.left;
				}
				break;

				case FontCodeType::Cursor:
				case FontCodeType::Tab:
					fillGlyph(info, charMaskWhite, charMask.find(info.codePoint)->second, j->first, _texBuffer, _texWidth, _texHeight, texX, texY);
					break;

				default:
                    {
                        renderGlyph<Outline>(_ftLibrary,info,j->first,_texBuffer,_texWidth,_texHeight,texX,texY,i->first,ftBitmap);
                        
                    }
                    break;
				}
			}
		}

		FT_Bitmap_Done(_ftLibrary, &ftBitmap);
	}

	void ResourceTrueTypeFont::fillGlyph(GlyphInfo& _info, uint8 _luminance, uint8 _alpha, int _lineHeight, uint8* _texBuffer, int _texWidth, int _texHeight, int& _texX, int& _texY)
	{
		int width = (int)ceil(_info.width);
		int height = (int)ceil(_info.height);

		autoWrapGlyphPos(width, _texWidth, _lineHeight, _texX, _texY);

		uint8* dest = _texBuffer + (_texY * _texWidth + _texX) * PixelBase::getNumBytes();

		// Calculate how much to advance the destination pointer after each row to get to the start of the next row.
		ptrdiff_t destNextRow = (_texWidth - width) * PixelBase::getNumBytes();

        
        
		for (int j = height; j > 0; --j)
		{
            for (int i=0;i<width;++i) {
                PixelBase::set(dest, _luminance, _alpha);
            }
			dest += destNextRow;
		}

		// Calculate and store the glyph's UV coordinates within the texture.
		_info.uvRect.left = (float)_texX / _texWidth; // u1
		_info.uvRect.top = (float)_texY / _texHeight; // v1
		_info.uvRect.right = (float)(_texX + _info.width) / _texWidth; // u2
		_info.uvRect.bottom = (float)(_texY + _info.height) / _texHeight; // v2
       
        if (width > 0)
			_texX += mGlyphSpacing + width;
	}
    
    void ResourceTrueTypeFont::putGlyph(GlyphInfo& _info,  int _lineHeight, uint8* _texBuffer, int _texWidth, int _texHeight, int& _texX, int& _texY,const FT_Bitmap* _bitmap,const Colour& _clr) {
        int width = (int)ceil(_info.width);
        int height = (int)ceil(_info.height);
        
        autoWrapGlyphPos(width, _texWidth, _lineHeight, _texX, _texY);
        
        uint8* dest = _texBuffer + (_texY * _texWidth + _texX ) * PixelBase::getNumBytes();
        
        // Calculate how much to advance the destination pointer after each row to get to the start of the next row.
        ptrdiff_t destNextRow = (_texWidth - _bitmap->width) * PixelBase::getNumBytes();
        
        const unsigned char* src = _bitmap->buffer;
        
        for (int j = _bitmap->rows; j > 0; --j)
        {
            for (int i=0;i<_bitmap->width;++i) {
                PixelBase::set(dest, _clr, *src++);
            }
            dest += destNextRow;
        }
        
        // Calculate and store the glyph's UV coordinates within the texture.
        _info.uvRect.left = (float)_texX / _texWidth; // u1
        _info.uvRect.top = (float)_texY / _texHeight; // v1
        _info.uvRect.right = (float)(_texX + _info.width) / _texWidth; // u2
        _info.uvRect.bottom = (float)(_texY + _info.height) / _texHeight; // v2
        
        if (width > 0)
            _texX += mGlyphSpacing + width;
    }
    
    void ResourceTrueTypeFont::blendGlyph(GlyphInfo& _info, int _dx,int _dy, int _lineHeight, uint8* _texBuffer, int _texWidth, int _texHeight, int& _texX, int& _texY,const FT_Bitmap* _bitmap,const Colour& _clr) {
        int width = (int)ceil(_info.width);
        int height = (int)ceil(_info.height);
        
        autoWrapGlyphPos(width, _texWidth, _lineHeight, _texX, _texY);
        
        uint8* dest = _texBuffer + ((_texY+_dy) * _texWidth + _texX + _dx) * PixelBase::getNumBytes();
        
        // Calculate how much to advance the destination pointer after each row to get to the start of the next row.
        ptrdiff_t destNextRow = (_texWidth - _bitmap->width) * PixelBase::getNumBytes();
        
        const unsigned char* src = _bitmap->buffer;
        
        for (int j = _bitmap->rows; j > 0; --j)
        {
            for (int i=0;i<_bitmap->width;++i) {
                PixelBase::blend(dest, _clr, *src++);
            }
            dest += destNextRow;
        }
    }
    
    template <>
    void ResourceTrueTypeFont::renderGlyph<false>(const FT_Library& _ftLibrary,MyGUI::GlyphInfo &_info, int _lineHeight, uint8 *_texBuffer, int _texWidth, int _texHeight, int &_texX, int &_texY, FT_UInt _glyphIndex, FT_Bitmap& _ftBitmap) {
        std::map<FT_UInt,FT_Bitmap>::const_iterator it = m_bitmaps_map.find(_glyphIndex);
        if (it != m_bitmaps_map.end()) {
            const FT_Bitmap& bitmap(it->second);
            if (bitmap.buffer != nullptr)
            {
                const FT_Bitmap* bitmap_buffer = 0;
                
                switch (bitmap.pixel_mode)
                {
                    case FT_PIXEL_MODE_GRAY:
                        bitmap_buffer = &bitmap;
                        break;
                        
                    case FT_PIXEL_MODE_MONO:
                        // Convert the monochrome bitmap to 8-bit before rendering it.
                        if (FT_Bitmap_Convert(_ftLibrary, &bitmap, &_ftBitmap, 1) == 0)
                        {
                            // Go through the bitmap and convert all of the nonzero values to 0xFF (white).
                            for (uint8* p = _ftBitmap.buffer, * endP = p + _ftBitmap.width * _ftBitmap.rows; p != endP; ++p)
                                *p ^= -*p ^ *p;
                            
                            bitmap_buffer = &_ftBitmap;
                        }
                        break;
                }
                if (bitmap_buffer) {
                    putGlyph(_info,_lineHeight,_texBuffer,_texWidth,_texHeight,_texX,_texY,bitmap_buffer,Colour::White);
                }
            }
            
        } else {
            MYGUI_LOG(Warning, "ResourceTrueTypeFont: Cannot rendered glyph " << _glyphIndex << " for character " << _info.codePoint << " in font '" << getResourceName() << "'.");
        }
    }
    
    template <>
    void ResourceTrueTypeFont::renderGlyph<true>(const FT_Library& _ftLibrary,MyGUI::GlyphInfo &_info, int _lineHeight, uint8 *_texBuffer, int _texWidth, int _texHeight, int &_texX, int &_texY, FT_UInt _glyphIndex, FT_Bitmap& _ftBitmap) {
        
        std::map<FT_UInt,FT_Glyph>::const_iterator it = m_outline_bitmaps_map.find(_glyphIndex);
        if (it != m_outline_bitmaps_map.end()) {
            FT_BitmapGlyph glyph = (FT_BitmapGlyph)it->second;
            
            
            if (glyph->bitmap.buffer != nullptr)
            {
                const FT_Bitmap* bitmap_buffer = 0;
                
                switch (glyph->bitmap.pixel_mode)
                {
                    case FT_PIXEL_MODE_GRAY:
                        bitmap_buffer = &(glyph->bitmap);
                        break;
                        
                    case FT_PIXEL_MODE_MONO:
                        // Convert the monochrome bitmap to 8-bit before rendering it.
                        if (FT_Bitmap_Convert(_ftLibrary, &(glyph->bitmap), &_ftBitmap, 1) == 0)
                        {
                            // Go through the bitmap and convert all of the nonzero values to 0xFF (white).
                            for (uint8* p = _ftBitmap.buffer, * endP = p + _ftBitmap.width * _ftBitmap.rows; p != endP; ++p)
                                *p ^= -*p ^ *p;
                            
                            bitmap_buffer = &_ftBitmap;
                        }
                        break;
                }
                int texX = _texX;
                int texY = _texY;
                if (bitmap_buffer) {
                    putGlyph(_info,_lineHeight,_texBuffer,_texWidth,_texHeight,_texX,_texY,bitmap_buffer,mOutlineColour);
                }
                
#if 1
                std::map<FT_UInt,FT_Bitmap>::const_iterator it = m_bitmaps_map.find(_glyphIndex);
                if (it != m_bitmaps_map.end()) {
                    const FT_Bitmap& bitmap(it->second);
                    if (bitmap.buffer != nullptr)
                    {
                        const FT_Bitmap* bitmap_buffer = 0;
                        
                        switch (bitmap.pixel_mode)
                        {
                            case FT_PIXEL_MODE_GRAY:
                                bitmap_buffer = &bitmap;
                                break;
                                
                            case FT_PIXEL_MODE_MONO:
                                // Convert the monochrome bitmap to 8-bit before rendering it.
                                if (FT_Bitmap_Convert(_ftLibrary, &bitmap, &_ftBitmap, 1) == 0)
                                {
                                    // Go through the bitmap and convert all of the nonzero values to 0xFF (white).
                                    for (uint8* p = _ftBitmap.buffer, * endP = p + _ftBitmap.width * _ftBitmap.rows; p != endP; ++p)
                                        *p ^= -*p ^ *p;
                                    
                                    bitmap_buffer = &_ftBitmap;
                                }
                                break;
                        }
                        if (bitmap_buffer) {
                            blendGlyph(_info,-glyph->left,
                                       glyph->top,_lineHeight,_texBuffer,_texWidth,_texHeight,texX,texY,bitmap_buffer,Colour::White);
                        }
                    }
                    
                } else {
                    MYGUI_LOG(Warning, "ResourceTrueTypeFont: Cannot rendered glyph " << _glyphIndex << " for character " << _info.codePoint << " in font '" << getResourceName() << "'.");
                }
#endif
                
            }
            
        } else {
            MYGUI_LOG(Warning, "ResourceTrueTypeFont: Cannot rendered glyph " << _glyphIndex << " for character " << _info.codePoint << " in font '" << getResourceName() << "'.");
        }
    }
    
 
	void ResourceTrueTypeFont::setSource(const std::string& _value)
	{
		mSource = _value;
	}

	void ResourceTrueTypeFont::setSize(float _value)
	{
		mSize = _value;
	}
    
    void ResourceTrueTypeFont::setScale(float _value) {
        mScale = _value;
    }

	void ResourceTrueTypeFont::setResolution(uint _value)
	{
		mResolution = _value;
	}

	void ResourceTrueTypeFont::setHinting(const std::string& _value)
	{
		if (_value == "use_native")
			mHinting = HintingUseNative;
		else if (_value == "force_auto")
			mHinting = HintingForceAuto;
		else if (_value == "disable_auto")
			mHinting = HintingDisableAuto;
		else if (_value == "disable_all")
			mHinting = HintingDisableAll;
		else
			mHinting = HintingUseNative;
	}
    
    void ResourceTrueTypeFont::setOutline(bool _value) {
        mOutline = _value;
    }
    
    void ResourceTrueTypeFont::setOutlineColour(const MyGUI::Colour &_value) {
        mOutlineColour = _value;
    }
    
    void ResourceTrueTypeFont::setOutlineWidth(float _value) {
        mOutlineWidth = _value;
    }

	void ResourceTrueTypeFont::setTabWidth(float _value)
	{
		mTabWidth = _value;
	}

	void ResourceTrueTypeFont::setOffsetHeight(int _value)
	{
		mOffsetHeight = _value;
	}

	void ResourceTrueTypeFont::setSubstituteCode(int _value)
	{
		mSubstituteCodePoint = _value;
	}

	void ResourceTrueTypeFont::setDistance(int _value)
	{
		mGlyphSpacing = _value;
	}


} // namespace MyGUI

#endif // MYGUI_USE_FREETYPE
