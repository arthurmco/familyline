/* Base class for all GUI controls
 *
 *  Copyright (C) 2018 Arthur Mendes
 */

#ifndef GUICONTROL_H
#define GUICONTROL_H

#include <queue>
#include <typeinfo>
#include <cairo/cairo.h>

#include "GUISignal.hpp"
#include <Log.hpp>

namespace Familyline::Graphics::GUI {

	/* The default type for GUI canvas */
	typedef cairo_surface_t* GUICanvas;

	/* THe GUI drawing context, where the used API saves its things */
	typedef cairo_t* GUIContext;

	/**
	 * Base class for GUI controls
	 *
	 * Defines just the base for our control to be identifiable (and maybe renderable? )
	 */
	class GUIControl {
	protected:
		// How to draw
		GUICanvas canvas = nullptr;
		GUIContext ctxt = nullptr;

		bool dirty = true;

		std::queue<GUISignal> signals;

		GUISignal receiveSignal();

		/* The function that does the real render
		   It needs to be pure.
		   If you want to do impure things, override the render function,
		   or simply don't do them here.
		*/
		virtual GUICanvas doRender(int absw, int absh) const = 0;

	public:
		// Positional
		float width, height;
		float x, y;

		/* Z-index. Smaller value means less rendering priority
		 * aka your control will be behind others
		 */
		int z_index = 0;

		/* Update the drawing context.
		 *
		 * This is designed under Cairo API. The context (cairo_t) is used for drawing, and
		 * it can only be created by the cairo_create. The cairo_create takes a surface
		 * (the GUICanvas, cairo_surface_t). This means a context is binded to a surface
		 *
		 * absw and absh are the absolute width and height of the whole window, not the control
		 */
		void setContext(unsigned absw, unsigned absh);

		/**
		 * Try to handle the signals. Returns true if handled
		 */
		virtual bool processSignal(GUISignal s);

		bool hasSignal();

		virtual void update();

		/* Start the rendering process.
		   Just calls doRender() and set the dirty status to false.
		   This function only exists so we can call render() inside tests.
		*/
		virtual void render(int absw, int absh);

		bool isDirty() const;

		GUICanvas getGUICanvas() const;

		void sendSignal(GUISignal s);

		virtual ~GUIControl();

	};


}


#endif /* GUICONTROL_H */
