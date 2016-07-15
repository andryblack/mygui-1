/*
 * This source file is part of MyGUI. For the latest info, see http://mygui.info/
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#ifndef MYGUI_I_RENDER_TARGET_H_
#define MYGUI_I_RENDER_TARGET_H_

#include "MyGUI_Prerequest.h"
#include <stddef.h>

namespace MyGUI
{

	class ITexture;
    struct Vertex;
    struct VertexQuad;
	
	class MYGUI_EXPORT IRenderTarget
	{
	public:
		IRenderTarget() { }
		virtual ~IRenderTarget() { }

		virtual void begin() = 0;
		virtual void end() = 0;

        virtual void setTexture(ITexture* _texture) = 0;
        virtual void addVertex(const Vertex& v) = 0;
        virtual void addQuad(const VertexQuad& q) = 0;
		
    };

} // namespace MyGUI

#endif // MYGUI_I_RENDER_TARGET_H_
