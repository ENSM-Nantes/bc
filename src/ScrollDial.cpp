// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IGUISkin.h"
#include "IGUIEnvironment.h"
#include "IVideoDriver.h"
#include "IGUIFont.h"
#include "IGUIFontBitmap.h"

#include "ScrollDial.h"

namespace irr
{
namespace gui
{


//! constructor
ScrollDial::ScrollDial(core::position2d< s32 > centre, u32 radius, IGUIEnvironment* environment,
				IGUIElement* parent, s32 id, s32 maxAngle, bool showValue, bool noclip) :
				IGUIScrollBar(environment, parent, id, core::rect<s32>(centre.X - radius, centre.Y - radius, centre.X + radius,centre.Y + radius)),
				centre(centre), radius(radius), maxAngle(maxAngle), showValue(showValue),
				Dragging(false), Pos(0), DrawPos(0), DrawAngle(0), DrawHeight(0),
				Min(0), Max(100), SmallStep(10), LargeStep(50), DesiredPos(0)
{

	#ifdef _DEBUG
	setDebugName("ScrollDial");
	#endif

//	refreshControls();

	setNotClipped(noclip);

	// this element can be tabbed to
	setTabStop(true);
	setTabOrder(-1);

	setPos(0);

	//Threshold angle is half way between the max angle and 360 degrees. Input between maxAngle and theshold goes to max, above theshold goes to 0
	thresholdAngle = (maxAngle + 360) / 2;
	startAngleDeg = 0;
}


//! destructor
ScrollDial::~ScrollDial()
{

}


//! called if an event happened.
bool ScrollDial::OnEvent(const SEvent& event)
{
	if (isEnabled())
	{

		switch(event.EventType)
		{
		case EET_KEY_INPUT_EVENT:
			if (event.KeyInput.PressedDown)
			{
				const s32 oldPos = Pos;
				bool absorb = true;
				switch (event.KeyInput.Key)
				{
				case KEY_LEFT:
				case KEY_UP:
					setPos(Pos-SmallStep);
					break;
				case KEY_RIGHT:
				case KEY_DOWN:
					setPos(Pos+SmallStep);
					break;
				case KEY_HOME:
					setPos(Min);
					break;
				case KEY_PRIOR:
					setPos(Pos-LargeStep);
					break;
				case KEY_END:
					setPos(Max);
					break;
				case KEY_NEXT:
					setPos(Pos+LargeStep);
					break;
				default:
					absorb = false;
				}

				if (Pos != oldPos)
				{
					SEvent newEvent;
					newEvent.EventType = EET_GUI_EVENT;
					newEvent.GUIEvent.Caller = this;
					newEvent.GUIEvent.Element = 0;
					newEvent.GUIEvent.EventType = EGET_SCROLL_BAR_CHANGED;
					Parent->OnEvent(newEvent);
				}
				if (absorb)
					return true;
			}
			break;
		case EET_GUI_EVENT:
			if (event.GUIEvent.EventType == EGET_ELEMENT_FOCUS_LOST)
			{
				if (event.GUIEvent.Caller == this)
					Dragging = false;
			}
			break;
		case EET_MOUSE_INPUT_EVENT:
		{
			const core::position2di p(event.MouseInput.X, event.MouseInput.Y);
			bool isInside = isPointInside ( p );
			switch(event.MouseInput.Event)
			{
			case EMIE_MOUSE_WHEEL:
				if (Environment->hasFocus(this))
				{
					// thanks to a bug report by REAPER
					// thanks to tommi by tommi for another bugfix
					// everybody needs a little thanking. hallo niko!;-)
					setPos(	getPos() +
							( (event.MouseInput.Wheel < 0 ? -1 : 1) * SmallStep )
							);

					SEvent newEvent;
					newEvent.EventType = EET_GUI_EVENT;
					newEvent.GUIEvent.Caller = this;
					newEvent.GUIEvent.Element = 0;
					newEvent.GUIEvent.EventType = EGET_SCROLL_BAR_CHANGED;
					Parent->OnEvent(newEvent);
					return true;
				}
				break;
			case EMIE_LMOUSE_PRESSED_DOWN:
			case EMIE_RMOUSE_PRESSED_DOWN: //JAMES: Allow right click for scroll bar movement
			{
				if (isInside)
				{
					Dragging = true;
					//DraggedBySlider = SliderRect.isPointInside(p);
					//TrayClick = !DraggedBySlider;
					//DesiredPos = getPosFromMousePos(p);
					setPos(getPosFromMousePos(p));
					SEvent newEvent;
					newEvent.EventType = EET_GUI_EVENT;
					newEvent.GUIEvent.Caller = this;
					newEvent.GUIEvent.Element = 0;
					newEvent.GUIEvent.EventType = EGET_SCROLL_BAR_CHANGED;
					Parent->OnEvent(newEvent);
					Environment->setFocus ( this );
					return true;
				}
				break;
			}
			case EMIE_LMOUSE_LEFT_UP:
			case EMIE_RMOUSE_LEFT_UP: //JAMES: Allow right click for scroll bar movement
			case EMIE_MOUSE_MOVED:
			{
				if ( !event.MouseInput.isLeftPressed () && !event.MouseInput.isRightPressed ()  ) //JAMES: Allow right click for scroll bar movement
					Dragging = false;

				if ( !Dragging )
				{
					if ( event.MouseInput.Event == EMIE_MOUSE_MOVED )
						break;
					return isInside;
				}

				if ( event.MouseInput.Event == EMIE_LMOUSE_LEFT_UP || event.MouseInput.Event == EMIE_RMOUSE_LEFT_UP ) //JAMES: Allow right click for scroll bar movement
					Dragging = false;

				const s32 newPos = getPosFromMousePos(p);
				const s32 oldPos = Pos;
                setPos(newPos);

				if (Pos != oldPos && Parent)
				{
					SEvent newEvent;
					newEvent.EventType = EET_GUI_EVENT;
					newEvent.GUIEvent.Caller = this;
					newEvent.GUIEvent.Element = 0;
					newEvent.GUIEvent.EventType = EGET_SCROLL_BAR_CHANGED;
					Parent->OnEvent(newEvent);
				}
				return isInside;
			} break;

			default:
				break;
			}
		} break;
		default:
			break;
		}
	}

	return IGUIElement::OnEvent(event);
}

void ScrollDial::OnPostRender(u32 timeMs)
{

}

//! draws the element and its children
void ScrollDial::draw()
{
	if (!IsVisible)
		return;

	IGUISkin* skin = Environment->getSkin();
	u32 skinAlpha = 255;
	if (skin)
		skinAlpha = skin->getColor(gui::EGDC_3D_FACE).getAlpha();

	s32 offsetX = AbsoluteRect.LowerRightCorner.X - RelativeRect.LowerRightCorner.X;
	s32 offsetY = AbsoluteRect.LowerRightCorner.Y - RelativeRect.LowerRightCorner.Y;
	core::vector2d<s32> C(centre.X + offsetX, centre.Y + offsetY);

	irr::video::IVideoDriver* driver = Environment->getVideoDriver();
	SliderRect = AbsoluteRect;

	const s32 borderW = 3;
	const s32 iR      = (s32)radius - borderW;
	const s32 R       = (s32)radius;

	video::SColor bgColor(skinAlpha, 220, 220, 220);
	video::SColor borderColor(skinAlpha, 0, 0, 0);

	// Arc colour based on current proportion
	f32 prop = core::isnotzero(range())
	           ? core::clamp((f32)(Pos - Min) / range(), 0.0f, 1.0f) : 0.0f;
	u32 rv = (prop < 0.5f) ? (u32)(prop * 2.0f * 220) : 220u;
	u32 gv = (prop < 0.5f) ? 200u : (u32)((1.0f - (prop - 0.5f) * 2.0f) * 200);
	video::SColor arcColor(skinAlpha, rv, gv, 0);

	f32 startRad = startAngleDeg * core::DEGTORAD;
	bool hasArc  = core::isnotzero(range()) && DrawAngle > 0.001f;
	bool fullArc = DrawAngle >= 2.0f * core::PI - 0.01f;

	// 1. Interior fill (scanlines) — guaranteed gap-free
	for (s32 dy = -iR; dy <= iR; dy++) {
		s32 mx = (s32)sqrtf((f32)(iR*iR - dy*dy));
		if (mx <= 0) continue;

		if (!hasArc || fullArc) {
			driver->draw2DRectangle(fullArc ? arcColor : bgColor,
				core::rect<s32>(C.X - mx, C.Y + dy, C.X + mx + 1, C.Y + dy + 1));
		} else {
			// Grey baseline for the whole row, then overwrite arc spans
			driver->draw2DRectangle(bgColor,
				core::rect<s32>(C.X - mx, C.Y + dy, C.X + mx + 1, C.Y + dy + 1));

			s32 spanStart = INT_MAX;
			for (s32 dx = -mx; dx <= mx + 1; dx++) {
				bool inArc = false;
				if (dx <= mx && (dx != 0 || dy != 0)) {
					f32 a = atan2f((f32)dx, -(f32)dy);
					if (a < 0) a += 2.0f * core::PI;
					f32 rel = a - startRad;
					if (rel < 0) rel += 2.0f * core::PI;
					inArc = (rel <= DrawAngle);
				}
				if (inArc && spanStart == INT_MAX) {
					spanStart = dx;
				} else if (!inArc && spanStart != INT_MAX) {
					driver->draw2DRectangle(arcColor,
						core::rect<s32>(C.X + spanStart, C.Y + dy,
						                C.X + dx,        C.Y + dy + 1));
					spanStart = INT_MAX;
				}
			}
		}
	}

	// 2. Border ring (annular scanlines) — guaranteed gap-free
	for (s32 dy = -R; dy <= R; dy++) {
		s32 outerX = (dy*dy <= R*R) ? (s32)sqrtf((f32)(R*R - dy*dy)) : 0;
		if (outerX <= 0) continue;
		s32 innerX = (dy*dy <= iR*iR) ? (s32)sqrtf((f32)(iR*iR - dy*dy)) : 0;
		// Left arc segment
		driver->draw2DRectangle(borderColor,
			core::rect<s32>(C.X - outerX, C.Y + dy, C.X - innerX, C.Y + dy + 1));
		// Right arc segment
		driver->draw2DRectangle(borderColor,
			core::rect<s32>(C.X + innerX, C.Y + dy, C.X + outerX + 1, C.Y + dy + 1));
	}

	// 3. Needle — always drawn so the zero position is visible
	{
		f32 na = startRad + DrawAngle;
		core::vector2d<s32> tip(C.X + (s32)(iR * sinf(na)), C.Y - (s32)(iR * cosf(na)));
		video::SColor needleColor(skinAlpha, 0, 0, 0);
		for (s32 d = -1; d <= 1; d++)
			driver->draw2DLine({C.X + d, C.Y}, {tip.X + d, tip.Y}, needleColor);
	}

}


void ScrollDial::updateAbsolutePosition()
{
	IGUIElement::updateAbsolutePosition();
	// todo: properly resize
//	refreshControls();
	setPos ( Pos );
}

//!
s32 ScrollDial::getPosFromMousePos(const core::position2di &pos) const
{
	//Get the angle (range 0-315 degrees), and convert into output position
	s32 offsetX = AbsoluteRect.LowerRightCorner.X - RelativeRect.LowerRightCorner.X;
    s32 offsetY = AbsoluteRect.LowerRightCorner.Y - RelativeRect.LowerRightCorner.Y;

    s32 relX = pos.X - centre.X - offsetX;
    s32 relY = pos.Y - centre.Y - offsetY;

    f32 angle = atan2(relX,-1.0*relY)*core::RADTODEG;
    while (angle<0) {angle+=360;} //As atan2 gives -pi to +pi
    // Remove start offset so angle is relative to the dial's zero position
    angle -= (f32)startAngleDeg;
    while (angle < 0) {angle += 360;}
    if (angle > thresholdAngle) {
		//Closer to 0 than to max
		angle=0;
	} else if (angle > maxAngle) {
		//Above max
		angle=maxAngle;
	}
    f32 proportion = angle/maxAngle;
    return (s32) (proportion * range()) + Min;
}


//! sets the position of the scrollbar
void ScrollDial::setPos(s32 pos)
{
	Pos = core::s32_clamp ( pos, Min, Max );

	f32 f = RelativeRect.getHeight()/ range();

    DrawPos = (s32)(( Pos - Min ) * f);
    DrawAngle = (Pos-Min) * maxAngle / range() * core::DEGTORAD; //0-315 degrees for display
    DrawHeight = RelativeRect.getWidth();


}


//! gets the small step value
s32 ScrollDial::getSmallStep() const
{
	return SmallStep;
}


//! sets the small step value
void ScrollDial::setSmallStep(s32 step)
{
	if (step > 0)
		SmallStep = step;
	else
		SmallStep = 10;
}


//! gets the small step value
s32 ScrollDial::getLargeStep() const
{
	return LargeStep;
}


//! sets the small step value
void ScrollDial::setLargeStep(s32 step)
{
	if (step > 0)
		LargeStep = step;
	else
		LargeStep = 50;
}


//! gets the maximum value of the scrollbar.
s32 ScrollDial::getMax() const
{
	return Max;
}


//! sets the maximum value of the scrollbar.
void ScrollDial::setMax(s32 max)
{
	Max = max;
	if ( Min > Max )
		Min = Max;

//	bool enable = core::isnotzero ( range() );
	setPos(Pos);
}

//! gets the minimum value of the scrollbar.
s32 ScrollDial::getMin() const
{
	return Min;
}


//! sets the minimum value of the scrollbar.
void ScrollDial::setMin(s32 min)
{
	Min = min;
	if ( Max < Min )
		Max = Min;


//	bool enable = core::isnotzero ( range() );
	setPos(Pos);
}


//! gets the current position of the scrollbar
s32 ScrollDial::getPos() const
{
	return Pos;
}

/*
//! refreshes the position and text on child buttons
void ScrollDial::refreshControls()
{
	CurrentIconColor = video::SColor(255,255,255,255);

	IGUISkin* skin = Environment->getSkin();

	if (skin)
	{
		CurrentIconColor = skin->getColor(isEnabled() ? EGDC_WINDOW_SYMBOL : EGDC_GRAY_WINDOW_SYMBOL);
	}

	if (Horizontal)
	{
		s32 h = RelativeRect.getHeight();
	}
	else
	{
		s32 w = RelativeRect.getWidth();
	}
}
*/

/*
//! Writes attributes of the element.
void ScrollDial::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options=0) const
{
	IGUIScrollBar::serializeAttributes(out,options);

	out->addBool("Horizontal",	Horizontal);
	out->addInt ("Value",		Pos);
	out->addInt ("Min",			Min);
	out->addInt ("Max",			Max);
	out->addInt ("SmallStep",	SmallStep);
	out->addInt ("LargeStep",	LargeStep);
	// CurrentIconColor - not serialized as continuiously updated
}


//! Reads attributes of the element
void ScrollDial::deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options=0)
{
	IGUIScrollBar::deserializeAttributes(in,options);

	Horizontal = in->getAttributeAsBool("Horizontal");
	setMin(in->getAttributeAsInt("Min"));
	setMax(in->getAttributeAsInt("Max"));
	setPos(in->getAttributeAsInt("Value"));
	setSmallStep(in->getAttributeAsInt("SmallStep"));
	setLargeStep(in->getAttributeAsInt("LargeStep"));
	// CurrentIconColor - not serialized as continuiously updated

	refreshControls();
}
*/

} // end namespace gui
} // end namespace irr

