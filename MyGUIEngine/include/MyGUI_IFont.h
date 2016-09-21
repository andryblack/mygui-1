/*
 * This source file is part of MyGUI. For the latest info, see http://mygui.info/
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#ifndef MYGUI_I_FONT_H_
#define MYGUI_I_FONT_H_

#include "MyGUI_Prerequest.h"
#include "MyGUI_ISerializable.h"
#include "MyGUI_IResource.h"
#include "MyGUI_FontData.h"

namespace MyGUI
{

	class ITexture;
    struct Colour;

	class MYGUI_EXPORT IFont :
		public IResource
	{
		MYGUI_RTTI_DERIVED( IFont )

	public:
		IFont() { }
		virtual ~IFont() { }

        virtual size_t getNumPasses() const { return 1; }
		virtual GlyphInfo* getGlyphInfo( int pass, Char _id) = 0;
        virtual const GlyphInfo* getSubstituteGlyphInfo() const { return 0; }
        bool hasGlyph(Char _id) {
            return getGlyphInfo(-1, _id)!=getSubstituteGlyphInfo();
        }

		virtual int getDefaultHeight() = 0;
        virtual std::string getPassName(size_t pass) { return ""; }
        virtual FloatSize getOffset( size_t pass ) { return FloatSize(0.0f,0.0f); }
   };

} // namespace MyGUI

#endif // MYGUI_I_FONT_H_
