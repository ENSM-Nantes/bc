
/*
  Copyright (C) 2014 James Packer

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License version 2 as
  published by the Free Software Foundation

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY Or FITNESS For A PARTICULAR PURPOSE.  See the
  GNU General Public License For more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

// DEE What does this do ... draws the GUI screen



#include <iostream>
#include <cmath>

#include "GUIMain.hpp"
#include "Constants.hpp"
#include "Utilities.hpp"
#include "ScrollDial.h"

GUIMain::GUIMain()
{

}

void GUIMain::load(irr::IrrlichtDevice* device, OwnShip *aOwnShip, Lines *aLines, Lang* language, std::vector<std::string>* logMessages, bool controlsHidden, bool showTideHeight, bool hasBowThruster, bool hasSternThruster, bool showCollided, bool vr3dMode)
{
  mOwnShip = aOwnShip;
  mLines = aLines;

  this->device = device;
  this->hasDepthSounder = mOwnShip->HasDepthSounder();
  this->maxSounderDepth = mOwnShip->GetMaxSounderDepth();
  this->hasGPS = mOwnShip->HasGPS();
  this->showTideHeight = showTideHeight;
  this->showCollided = showCollided;
  this->hasBowThruster = hasBowThruster;
  this->hasRateOfTurnIndicator = mOwnShip->HasRoTIndicator();
  this->controlsHidden = controlsHidden;

  this->hasSternThruster = hasSternThruster;
  guienv = device->getGUIEnvironment();

  irr::video::IVideoDriver* driver = device->getVideoDriver();
  su = driver->getScreenSize().Width;
  sh = driver->getScreenSize().Height;

  this->language = language;
  this->logMessages = logMessages;

	
  //Set gui skin less transparent
  irr::video::SColor col = guienv->getSkin()->getColor(irr::gui::EGDC_3D_SHADOW);
  col.setAlpha(200);
  guienv->getSkin()->setColor(irr::gui::EGDC_3D_SHADOW, col);

  col = guienv->getSkin()->getColor(irr::gui::EGDC_3D_FACE);
  col.setAlpha(200);
  guienv->getSkin()->setColor(irr::gui::EGDC_3D_FACE, col);

      
  azimuthGUIOffsetL = 0;
  azimuthGUIOffsetR = 0;
      

  //Initial settings for NFU buttons
  nfuPortDown = false;
  nfuStbdDown = false;

  //Default to small radar display
  radarLarge = false;
  //Find available 4:3 rectangle to fit in area for large radar display
  irr::s32 availableWidth;
  irr::s32 availableHeight = (0.95-0.01)*sh;

  // leave 0.9*su on left, 0.01*su on right
  availableWidth  = (0.99-0.09)*su;
       
  if (availableWidth/(float)availableHeight > 4.0/3.0) {
    // Wider than 4:3
    irr::s32 activeWidth = availableHeight * 4.0/3.0;
    irr::s32 activeHeight = availableHeight;
    radarLargeRect = irr::core::rect<irr::s32>(0.09*su + (availableWidth-activeWidth)/2, 0.01*sh, 0.09*su + activeWidth + (availableWidth-activeWidth)/2, 0.01+activeHeight);
  } else {
    // 4:3 or narrower
    irr::s32 activeWidth = availableWidth;
    irr::s32 activeHeight = availableWidth * 3.0/4.0;
    radarLargeRect = irr::core::rect<irr::s32>(0.09*su, 0.01*sh+(availableHeight-activeHeight)/2, 0.09*su + activeWidth, 0.01+activeHeight+(availableHeight-activeHeight)/2);
  }
  //For brevity, store large radar window width and top left corner.
  irr::s32 radarSu = radarLargeRect.getWidth();
  irr::core::vector2d<irr::s32> radarTL = radarLargeRect.UpperLeftCorner;
  //Find radar screen centre X, Y and radius
  largeRadarScreenRadius = (radarLargeRect.LowerRightCorner.Y-radarTL.Y)/2;
  largeRadarScreenCentreX = radarTL.X + largeRadarScreenRadius;
  largeRadarScreenCentreY = (radarLargeRect.LowerRightCorner.Y+radarTL.Y)/2;
  largeRadarScreenRadius*=0.95; //Make display slightly smaller, keeping the centre in the same place

  smallRadarScreenCentreX = su-0.2*sh+azimuthGUIOffsetR;
  smallRadarScreenCentreY = 0.8*sh;
  smallRadarScreenRadius=0.2*sh;

  //gui - add scroll bars for speed and heading control directly
  hdgScrollbar = new irr::gui::OutlineScrollBar(false,guienv,guienv->getRootGUIElement(),GUI_ID_HEADING_SCROLL_BAR,irr::core::rect<irr::s32>(0.01*su, 0.61*sh, 0.04*su, 0.99*sh));
  hdgScrollbar->setMax(360);
  spdScrollbar = new irr::gui::OutlineScrollBar(false,guienv,guienv->getRootGUIElement(),GUI_ID_SPEED_SCROLL_BAR,irr::core::rect<irr::s32>(0.05*su, 0.61*sh, 0.08*su, 0.99*sh));
  spdScrollbar->setMax(20.f*1852.f/3600.f); //20 knots in m/s
  //Hide speed/heading bars normally
  hdgScrollbar->setVisible(false);
  spdScrollbar->setVisible(false);

  //Add engine, rudder and thruster bars
  irr::core::array<irr::s32> rudderTics; rudderTics.push_back(-25);rudderTics.push_back(-20);rudderTics.push_back(-15);rudderTics.push_back(-10);rudderTics.push_back(-5);
  rudderTics.push_back(5);rudderTics.push_back(10);rudderTics.push_back(15);rudderTics.push_back(20);rudderTics.push_back(25);

  //Values to show on wheel control (should be same size as rudderTics, but we probably want to show an unsigned version in the GUI
  irr::core::array<irr::s32> rudderIndicatorTics; rudderIndicatorTics.push_back(25);rudderIndicatorTics.push_back(20);rudderIndicatorTics.push_back(15);rudderIndicatorTics.push_back(10);rudderIndicatorTics.push_back(5);
  rudderIndicatorTics.push_back(5);rudderIndicatorTics.push_back(10);rudderIndicatorTics.push_back(15);rudderIndicatorTics.push_back(20);rudderIndicatorTics.push_back(25);


  irr::core::array<irr::s32> engineTics; engineTics.push_back(-80);engineTics.push_back(-60);engineTics.push_back(-40);engineTics.push_back(-20);
  engineTics.push_back(20);engineTics.push_back(40);engineTics.push_back(60);engineTics.push_back(80);

  irr::core::array<irr::s32> centreTic; centreTic.push_back(0);

  // Vertical stack in this corner, top to bottom: thruster rows (if any) -> wheel -> data display -> interface buttons.
  // The button row is anchored to the true bottom of the screen; everything else builds upward from there.
  // stdGap is the single standard spacing used everywhere in this stack: between rows, between stations,
  // and (horizontally, further below) between a label and the control it names.
  // Row heights are kept small throughout this stack so a "Wheel" label above the wheel scrollbar
  // doesn't push the thruster rows up into the compass/RoT block above them.
  irr::f32 stdGap = 0.005;
  irr::f32 labelsRowHeight = 0.012;
  irr::f32 scrollRowHeight = 0.025;

  irr::f32 buttonRowBottom = 0.99f;
  irr::f32 buttonRowTop = buttonRowBottom - 0.03f;

  // Height a bow/stern row would take (scrollbar + its label, with gaps) - reclaimed into the data
  // display below when the corresponding thruster isn't fitted, so there's less need to scroll it.
  irr::f32 thrusterRowHeight = scrollRowHeight + stdGap + labelsRowHeight + stdGap;
  irr::f32 dataDisplayReclaimedHeight = (hasBowThruster ? 0.0f : thrusterRowHeight) + (hasSternThruster ? 0.0f : thrusterRowHeight);

  irr::f32 dataDisplayBottom = buttonRowTop - stdGap;
  irr::f32 dataDisplayTop = dataDisplayBottom - 0.05f - dataDisplayReclaimedHeight;

  irr::f32 wheelRowBottom = dataDisplayTop - stdGap;
  irr::f32 wheelRowTop = wheelRowBottom - 0.03f;

  irr::f32 wheelLabelHeight = 0.02f;
  irr::f32 wheelLabelBottom = wheelRowTop - stdGap;
  irr::f32 wheelLabelTop = wheelLabelBottom - wheelLabelHeight;

  irr::f32 nextRowBottom = wheelLabelTop - stdGap; // top of the "Wheel" label, minus the gap

  // Each station (if present) is a name label ("Bow"/"Stern") above a single bidirectional scrollbar,
  // split red (port, left half)/green (stbd, right half) - same style as the wheel scrollbar.
  irr::f32 sternLabelTop=0, sternLabelBottom=0, sternScrollTop=0, sternScrollBottom=0;
  if (hasSternThruster) {
    sternScrollBottom = nextRowBottom;
    sternScrollTop = sternScrollBottom - scrollRowHeight;
    sternLabelBottom = sternScrollTop - stdGap;
    sternLabelTop = sternLabelBottom - labelsRowHeight;
    nextRowBottom = sternLabelTop - stdGap;
  }

  irr::f32 bowLabelTop=0, bowLabelBottom=0, bowScrollTop=0, bowScrollBottom=0;
  if (hasBowThruster) {
    bowScrollBottom = nextRowBottom;
    bowScrollTop = bowScrollBottom - scrollRowHeight;
    bowLabelBottom = bowScrollTop - stdGap;
    bowLabelTop = bowLabelBottom - labelsRowHeight;
    nextRowBottom = bowLabelTop - stdGap;
  }

  {
    // Color zones: left half = port (red), right half = stbd (green) - same style as the wheel scrollbar
    irr::core::array<irr::s32> thrusterZoneThresholds;
    irr::core::array<irr::video::SColor> thrusterZoneColors;
    thrusterZoneThresholds.push_back(-100); thrusterZoneColors.push_back(irr::video::SColor(160, 200,  30,  30)); // port = red
    thrusterZoneThresholds.push_back(   0); thrusterZoneColors.push_back(irr::video::SColor(160,  30, 180,  30)); // stbd = green

    if (hasBowThruster) {
      bowThrusterNameLabel = guienv->addStaticText(language->translate("bow").c_str(),irr::core::rect<irr::s32>(0.13*su, bowLabelTop*sh, 0.45*su, bowLabelBottom*sh));
      bowThrusterNameLabel->setTextAlignment(irr::gui::EGUIA_CENTER,irr::gui::EGUIA_CENTER);

      bowThrusterScrollbar = new irr::gui::OutlineScrollBar(true,guienv,guienv->getRootGUIElement(),GUI_ID_BOWTHRUSTER_SCROLL_BAR,irr::core::rect<irr::s32>(0.13*su, bowScrollTop*sh, 0.45*su, bowScrollBottom*sh),engineTics,centreTic);
      bowThrusterScrollbar->setMax(100);
      bowThrusterScrollbar->setMin(-100);
      bowThrusterScrollbar->setPos(0);
      bowThrusterScrollbar->setToolTipText(language->translate("bowThruster").c_str());
      static_cast<irr::gui::OutlineScrollBar*>(bowThrusterScrollbar)->setColorZones(thrusterZoneThresholds, thrusterZoneColors);

      // Left/right step buttons, same position/style as the wheel's NFU port/stbd buttons
      bowThrusterLeftButton = guienv->addButton(irr::core::rect<irr::s32>(0.09*su, bowScrollTop*sh, 0.11*su, bowScrollBottom*sh),0,GUI_ID_BOWTHRUSTER_LEFT_BUTTON,language->translate("NFUPort").c_str());
      bowThrusterLeftButton->setToolTipText(language->translate("bowThrusterPort").c_str());
      bowThrusterRightButton = guienv->addButton(irr::core::rect<irr::s32>(0.11*su, bowScrollTop*sh, 0.13*su, bowScrollBottom*sh),0,GUI_ID_BOWTHRUSTER_RIGHT_BUTTON,language->translate("NFUStbd").c_str());
      bowThrusterRightButton->setToolTipText(language->translate("bowThrusterStbd").c_str());
    } else {
      bowThrusterScrollbar = 0;
      bowThrusterNameLabel = 0;
      bowThrusterLeftButton = 0;
      bowThrusterRightButton = 0;
    }

    if (hasSternThruster) {
      sternThrusterNameLabel = guienv->addStaticText(language->translate("stern").c_str(),irr::core::rect<irr::s32>(0.13*su, sternLabelTop*sh, 0.45*su, sternLabelBottom*sh));
      sternThrusterNameLabel->setTextAlignment(irr::gui::EGUIA_CENTER,irr::gui::EGUIA_CENTER);

      sternThrusterScrollbar = new irr::gui::OutlineScrollBar(true,guienv,guienv->getRootGUIElement(),GUI_ID_STERNTHRUSTER_SCROLL_BAR,irr::core::rect<irr::s32>(0.13*su, sternScrollTop*sh, 0.45*su, sternScrollBottom*sh),engineTics,centreTic);
      sternThrusterScrollbar->setMax(100);
      sternThrusterScrollbar->setMin(-100);
      sternThrusterScrollbar->setPos(0);
      sternThrusterScrollbar->setToolTipText(language->translate("sternThruster").c_str());
      static_cast<irr::gui::OutlineScrollBar*>(sternThrusterScrollbar)->setColorZones(thrusterZoneThresholds, thrusterZoneColors);

      // Left/right step buttons, same position/style as the wheel's NFU port/stbd buttons
      sternThrusterLeftButton = guienv->addButton(irr::core::rect<irr::s32>(0.09*su, sternScrollTop*sh, 0.11*su, sternScrollBottom*sh),0,GUI_ID_STERNTHRUSTER_LEFT_BUTTON,language->translate("NFUPort").c_str());
      sternThrusterLeftButton->setToolTipText(language->translate("sternThrusterPort").c_str());
      sternThrusterRightButton = guienv->addButton(irr::core::rect<irr::s32>(0.11*su, sternScrollTop*sh, 0.13*su, sternScrollBottom*sh),0,GUI_ID_STERNTHRUSTER_RIGHT_BUTTON,language->translate("NFUStbd").c_str());
      sternThrusterRightButton->setToolTipText(language->translate("sternThrusterStbd").c_str());
    } else {
      sternThrusterScrollbar = 0;
      sternThrusterNameLabel = 0;
      sternThrusterLeftButton = 0;
      sternThrusterRightButton = 0;
    }
  }

  portText = guienv->addStaticText(language->translate("portEngine").c_str(),irr::core::rect<irr::s32>(0.005*su, 0.61*sh, 0.045*su, 0.67*sh));
  portText->setTextAlignment(irr::gui::EGUIA_CENTER,irr::gui::EGUIA_CENTER);
  portText->setOverrideColor(irr::video::SColor(255,128,0,0));
  portScrollbar = new irr::gui::OutlineScrollBar(false,guienv,guienv->getRootGUIElement(),GUI_ID_PORT_SCROLL_BAR,irr::core::rect<irr::s32>(0.01*su, 0.675*sh, 0.04*su, 0.99*sh),engineTics,centreTic,true);
  portScrollbar->setMax(100);
  portScrollbar->setMin(-100);
  portScrollbar->setPos(0);
  stbdText = guienv->addStaticText(language->translate("stbdEngine").c_str(),irr::core::rect<irr::s32>(0.045*su, 0.61*sh, 0.085*su, 0.67*sh));
  stbdText->setTextAlignment(irr::gui::EGUIA_CENTER,irr::gui::EGUIA_CENTER);
  stbdText->setOverrideColor(irr::video::SColor(255,0,128,0));
  stbdScrollbar = new irr::gui::OutlineScrollBar(false,guienv,guienv->getRootGUIElement(),GUI_ID_STBD_SCROLL_BAR,irr::core::rect<irr::s32>(0.05*su, 0.675*sh, 0.08*su, 0.99*sh),engineTics,centreTic,true);
  stbdScrollbar->setMax(100);
  stbdScrollbar->setMin(-100);
  stbdScrollbar->setPos(0);

  // Color zones: scroll pos -100→-10 = top = ahead (green), -10→+10 = stop (yellow), +10→100 = bottom = astern (red)
  {
    irr::core::array<irr::s32> ezThresholds;
    irr::core::array<irr::video::SColor> ezColors;
    ezThresholds.push_back(-100);
    ezColors.push_back(irr::video::SColor(160,  30, 180,  30)); // ahead
    ezThresholds.push_back( 0);
    ezColors.push_back(irr::video::SColor(160, 200,  30,  30)); // astern
    static_cast<irr::gui::OutlineScrollBar*>(portScrollbar)->setColorZones(ezThresholds, ezColors);
    static_cast<irr::gui::OutlineScrollBar*>(stbdScrollbar)->setColorZones(ezThresholds, ezColors);
  }

  // Label sits on its own row above the wheel scrollbar, same style as the compass/RoT labels
  wheelLabel = guienv->addStaticText(language->translate("wheelText").c_str(),
    irr::core::rect<irr::s32>(0.13*su, wheelLabelTop*sh, 0.45*su, wheelLabelBottom*sh));
  wheelLabel->setTextAlignment(irr::gui::EGUIA_CENTER, irr::gui::EGUIA_CENTER);

  wheelScrollbar = new irr::gui::OutlineScrollBar(true,guienv,guienv->getRootGUIElement(),GUI_ID_WHEEL_SCROLL_BAR,irr::core::rect<irr::s32>(0.13*su, wheelRowTop*sh, 0.45*su, wheelRowBottom*sh),rudderTics,centreTic,true,rudderIndicatorTics);
  wheelScrollbar->setMax(30);
  wheelScrollbar->setMin(-30);
  wheelScrollbar->setPos(0);

  // Color zones: left = port (red), centre neutral, right = starboard (green)
  {
    irr::core::array<irr::s32> wzThresholds;
    irr::core::array<irr::video::SColor> wzColors;
    wzThresholds.push_back(-30); wzColors.push_back(irr::video::SColor(160, 200,  30,  30)); // port = red
    wzThresholds.push_back(  0); wzColors.push_back(irr::video::SColor(160,  30, 180,  30)); // starboard = green
    wheelScrollbar->setColorZones(wzThresholds, wzColors);
  }

  nonFollowUpPortButton = guienv->addButton(irr::core::rect<irr::s32>(0.09*su, wheelRowTop*sh, 0.11*su, wheelRowBottom*sh),0,GUI_ID_NFU_PORT_BUTTON,language->translate("NFUPort").c_str());
  nonFollowUpStbdButton = guienv->addButton(irr::core::rect<irr::s32>(0.11*su, wheelRowTop*sh, 0.13*su, wheelRowBottom*sh),0,GUI_ID_NFU_STBD_BUTTON,language->translate("NFUStbd").c_str());

  //Adapt if single engine:
  if (1 == mOwnShip->getNumberProp()) {
    stbdScrollbar->setVisible(false);
    stbdText->setVisible(false);

    //Get max extent of both engine scroll bars
    irr::core::vector2d<irr::s32> lowerRight = stbdScrollbar->getRelativePosition().LowerRightCorner;
    irr::core::vector2d<irr::s32> upperLeft = portScrollbar->getRelativePosition().UpperLeftCorner;
    portScrollbar->setRelativePosition(irr::core::rect<irr::s32>(upperLeft,lowerRight));

    //Change text from 'portEngine' to 'engine', and use all space
    portText->setText(language->translate("engine").c_str());
    portText->enableOverrideColor(false);
    lowerRight = stbdText->getRelativePosition().LowerRightCorner;
    upperLeft = portText->getRelativePosition().UpperLeftCorner;
    portText->setRelativePosition(irr::core::rect<irr::s32>(upperLeft,lowerRight));
  }

  //Add 'hint' text to click on the rudder and wheel controls
  clickForRudderText = guienv->addStaticText(language->translate("startupHelpRudder").c_str(),wheelScrollbar->getAbsolutePosition());
  clickForRudderText->setTextAlignment(irr::gui::EGUIA_CENTER,irr::gui::EGUIA_CENTER);
  clickForRudderText->setOverrideColor(irr::video::SColor(255,255,0,0));

  irr::core::rect<irr::s32> engineHintPos = irr::core::rect<irr::s32>(
								      portScrollbar->getRelativePosition().UpperLeftCorner,
								      stbdScrollbar->getRelativePosition().LowerRightCorner);

  clickForEngineText = guienv->addStaticText(language->translate("startupHelpEngine").c_str(),engineHintPos);
  clickForEngineText->setTextAlignment(irr::gui::EGUIA_CENTER,irr::gui::EGUIA_CENTER);
  clickForEngineText->setOverrideColor(irr::video::SColor(255,255,0,0));
	    

  //add data display:
  // Sits directly under the wheel scrollbar, left-aligned with the first interface button below it.
  // A list box (not a plain static text) so it gets a scrollbar automatically once the content overflows.
  stdDataDisplayPos = irr::core::rect<irr::s32>(0.09*su+azimuthGUIOffsetL,dataDisplayTop*sh,0.45*su+azimuthGUIOffsetR,dataDisplayBottom*sh); //In normal view
  radDataDisplayPos = irr::core::rect<irr::s32>(0.83*su,0.96*sh,0.99*su,0.99*sh); //In maximised 3d view
  altDataDisplayPos = irr::core::rect<irr::s32>(0.83*su,0.96*sh,0.99*su,0.99*sh); //In maximised 3d view
  dataDisplay = guienv->addListBox(stdDataDisplayPos, 0, -1, true); //Actual content set later
  dataDisplay->setAutoScrollEnabled(false); // Don't jump to the newest item - this list is rebuilt every frame

  guiHeading = 0;
  guiSpeed = 0;

  //Add heading indicator
  // This whole block starts right at the top of the control cluster (matching the reference y already
  // used by portText/stbdText/hdgScrollbar), so there's no dead space above it. Each element is its own
  // row, top to bottom: "Compass" label -> gauge -> "RoT" label -> RoT scrollbar, each separated by stdGap.
  // Kept compact (small label rows, smaller gauge/scrollbar) to leave more room below for the thruster rows.
  irr::f32 labelRowHeight2 = 0.02f;
  irr::f32 compassLabelTop = 0.61f;
  irr::f32 compassLabelBottom = compassLabelTop + labelRowHeight2;
  irr::f32 compassRowTop = compassLabelBottom + stdGap;
  irr::f32 compassRowBottom = compassRowTop + 0.04f;
  irr::f32 rotLabelTop = compassRowBottom + stdGap;
  irr::f32 rotLabelBottom = rotLabelTop + labelRowHeight2;
  irr::f32 rotRowTop = rotLabelBottom + stdGap;
  irr::f32 rotRowBottom = rotRowTop + 0.025f;

  stdHdgIndicatorPos = irr::core::rect<irr::s32>(0.09*su+azimuthGUIOffsetL,compassRowTop*sh,0.45*su+azimuthGUIOffsetR,compassRowBottom*sh); //In normal view
  radHdgIndicatorPos = irr::core::rect<irr::s32>(0.46*su, 0.96*sh, 0.82*su, 0.99*sh); //In maximised radar view
  maxHdgIndicatorPos = irr::core::rect<irr::s32>(0.46*su, 0.96*sh, 0.82*su, 0.99*sh); //In maximised 3d view

  // Cadet blue background behind the gauge (hidden in large radar mode)
  compassBG = guienv->addStaticText(L"",
    irr::core::rect<irr::s32>(0.09*su+azimuthGUIOffsetL, compassRowTop*sh, 0.45*su+azimuthGUIOffsetR, compassRowBottom*sh));
  compassBG->setBackgroundColor(irr::video::SColor(255, 95, 158, 160));
  compassBG->setDrawBackground(true);

  headingIndicator = new irr::gui::HeadingIndicator(guienv,guienv->getRootGUIElement(),stdHdgIndicatorPos);

  compassLabel = guienv->addStaticText(language->translate("compass").c_str(),
    irr::core::rect<irr::s32>(0.09*su+azimuthGUIOffsetL, compassLabelTop*sh, 0.45*su+azimuthGUIOffsetR, compassLabelBottom*sh));
  compassLabel->setTextAlignment(irr::gui::EGUIA_CENTER, irr::gui::EGUIA_CENTER);

  // DEE vvvvv add very basic rate of turn indicator
  // rewrite this with its own class so that it is more realistic i.e. either a dial or a conning display

  rateofturnScrollbar = new irr::gui::OutlineScrollBar(true,guienv,guienv->getRootGUIElement(),GUI_ID_RATE_OF_TURN_SCROLL_BAR,irr::core::rect<irr::s32>(0.09*su+azimuthGUIOffsetL, rotRowTop*sh, 0.45*su+azimuthGUIOffsetR, rotRowBottom*sh),rudderTics,centreTic);

  rateofturnScrollbar->setMax(50);
  rateofturnScrollbar->setMin(-50);
  rateofturnScrollbar->setSmallStep(1);
  rateofturnScrollbar->setPos(0);
  rateofturnScrollbar->setToolTipText(language->translate("rotText").c_str());

  // Color zones: left = port turn (red), centre = no turn, right = starboard turn (green)
  {
    irr::core::array<irr::s32> rotThresholds;
    irr::core::array<irr::video::SColor> rotColors;
    rotThresholds.push_back(-50); rotColors.push_back(irr::video::SColor(160, 200,  30,  30)); // port
    rotThresholds.push_back(  0); rotColors.push_back(irr::video::SColor(160,  30, 180,  30)); // starboard
    static_cast<irr::gui::OutlineScrollBar*>(rateofturnScrollbar)->setColorZones(rotThresholds, rotColors);
  }

  rateofturnText = guienv->addStaticText(L"RoT: 0.0 °/min",
    irr::core::rect<irr::s32>(0.09*su+azimuthGUIOffsetL, rotLabelTop*sh, 0.45*su+azimuthGUIOffsetR, rotLabelBottom*sh));
  rateofturnText->setTextAlignment(irr::gui::EGUIA_CENTER, irr::gui::EGUIA_CENTER);

  if (!hasRateOfTurnIndicator) {
    rateofturnScrollbar->setVisible(false);
    rateofturnText->setVisible(false);
  }

  // DEE ^^^^^


  //Add an additional window for controls (will normally be hidden)
  irr::core::rect<irr::s32> extraControlsWindowPos(
    0.09*su + azimuthGUIOffsetL, (irr::s32)(0.55*sh),
    0.45*su + azimuthGUIOffsetR, (irr::s32)(0.95*sh));
  extraControlsWindow=guienv->addWindow(extraControlsWindowPos);
  extraControlsWindow->getCloseButton()->setVisible(false);
  extraControlsWindow->setText(language->translate("extraControls").c_str());
  guienv->addButton(extraControlsWindow->getCloseButton()->getRelativePosition(),extraControlsWindow,GUI_ID_HIDE_EXTRA_CONTROLS_BUTTON,L"X");
  extraControlsWindow->setVisible(false);

  // Add tab control for extra settings like weather and machinery failure
  irr::core::rect<irr::s32> extraControlsTabPosition = irr::core::rect<irr::s32>(
										 0.01 * su,
										 0.05 * sh,
										 extraControlsWindowPos.getWidth() - 0.02 * su,
										 extraControlsWindowPos.getHeight() - 0.01 * sh
										 );
  irr::gui::IGUITabControl* extraControlsTabControl = guienv->addTabControl(extraControlsTabPosition, extraControlsWindow);
  extraControlsTabControl->setTabHeight(0.03*sh);
        
  // Weather tab
  irr::gui::IGUITab* extraControlsTabWeather = extraControlsTabControl->addTab(language->translate("weather").c_str());

  //Add weather scroll bar
  //weatherScrollbar = guienv->addScrollBar(false,irr::core::rect<irr::s32>(0.417*su, 0.79*sh, 0.440*su, 0.94*sh), 0, GUI_ID_WEATHER_SCROLL_BAR);
  guienv->addStaticText(language->translate("weather").c_str(),irr::core::rect<irr::s32>(0.005*su,0.02*sh,0.085*su,0.05*sh),false,true,extraControlsTabWeather)->setTextAlignment(irr::gui::EGUIA_CENTER,irr::gui::EGUIA_CENTER);
  weatherScrollbar = new irr::gui::ScrollDial(irr::core::vector2d<irr::s32>(0.045*su,0.09*sh),0.03*su,guienv,extraControlsTabWeather,GUI_ID_WEATHER_SCROLL_BAR);

  weatherScrollbar->setMax(120); //Divide by 10 to get weather
  weatherScrollbar->setMin(0);
  weatherScrollbar->setSmallStep(5);
  weatherScrollbar->setToolTipText(language->translate("weather").c_str());

  //Add rain scroll bar
  //rainScrollbar = guienv->addScrollBar(false,irr::core::rect<irr::s32>(0.389*su, 0.79*sh, 0.412*su, 0.94*sh), 0, GUI_ID_RAIN_SCROLL_BAR);
  guienv->addStaticText(language->translate("rain").c_str(),irr::core::rect<irr::s32>(0.085*su,0.02*sh,0.165*su,0.05*sh),false,true,extraControlsTabWeather)->setTextAlignment(irr::gui::EGUIA_CENTER,irr::gui::EGUIA_CENTER);
  rainScrollbar = new irr::gui::ScrollDial(irr::core::vector2d<irr::s32>(0.125*su,0.09*sh),0.03*su,guienv,extraControlsTabWeather,GUI_ID_RAIN_SCROLL_BAR);
  rainScrollbar->setMax(100);
  rainScrollbar->setMin(0);
  rainScrollbar->setLargeStep(5);
  rainScrollbar->setSmallStep(5);
  rainScrollbar->setToolTipText(language->translate("rain").c_str());

  //Add visibility scroll bar: Will be divided by 10 to get visibility in Nm
  //visibilityScrollbar = guienv->addScrollBar(false,irr::core::rect<irr::s32>(0.361*su, 0.79*sh, 0.384*su, 0.94*sh),0,GUI_ID_VISIBILITY_SCROLL_BAR);
  guienv->addStaticText(language->translate("visibility").c_str(),irr::core::rect<irr::s32>(0.165*su,0.02*sh,0.245*su,0.05*sh),false,true,extraControlsTabWeather)->setTextAlignment(irr::gui::EGUIA_CENTER,irr::gui::EGUIA_CENTER);
  visibilityScrollbar = new irr::gui::ScrollDial(irr::core::vector2d<irr::s32>(0.205*su,0.09*sh),0.03*su,guienv,extraControlsTabWeather,GUI_ID_VISIBILITY_SCROLL_BAR);
  visibilityScrollbar->setMax(101);
  visibilityScrollbar->setMin(1);
  visibilityScrollbar->setLargeStep(5);
  visibilityScrollbar->setSmallStep(1);
  visibilityScrollbar->setToolTipText(language->translate("visibility").c_str());

  // Wind direction and speed    
  windDirectionScrollbar = new irr::gui::ScrollDial(irr::core::vector2d<irr::s32>(0.26*su,0.06*sh),0.0175*su,guienv,extraControlsTabWeather,GUI_ID_WINDDIRECTION_SCROLL_BAR, 360, true);
  windDirectionScrollbar->setMax(360);
  windDirectionScrollbar->setMin(0);
  windDirectionScrollbar->setLargeStep(45);
  windDirectionScrollbar->setSmallStep(5);
  windDirectionScrollbar->setToolTipText(language->translate("windDirection").c_str());

  windSpeedScrollbar = new irr::gui::ScrollDial(irr::core::vector2d<irr::s32>(0.26*su,0.12*sh),0.0175*su,guienv,extraControlsTabWeather,GUI_ID_WINDSPEED_SCROLL_BAR,315, true);
  windSpeedScrollbar->setMax(50);
  windSpeedScrollbar->setMin(0);
  windSpeedScrollbar->setLargeStep(5);
  windSpeedScrollbar->setSmallStep(1);
  windSpeedScrollbar->setToolTipText(language->translate("windSpeed").c_str());

  // Tidal stream override
  streamDirectionScrollbar = new irr::gui::ScrollDial(irr::core::vector2d<irr::s32>(0.30*su,0.06*sh),0.0175*su,guienv,extraControlsTabWeather,GUI_ID_STREAMDIRECTION_SCROLL_BAR, 360, true);
  streamDirectionScrollbar->setMax(360);
  streamDirectionScrollbar->setMin(0);
  streamDirectionScrollbar->setLargeStep(45);
  streamDirectionScrollbar->setSmallStep(5);
  streamDirectionScrollbar->setToolTipText(language->translate("streamDirection").c_str());

  streamSpeedScrollbar = new irr::gui::ScrollDial(irr::core::vector2d<irr::s32>(0.30*su,0.12*sh),0.0175*su,guienv,extraControlsTabWeather,GUI_ID_STREAMSPEED_SCROLL_BAR, 315, true);
  streamSpeedScrollbar->setMax(10);
  streamSpeedScrollbar->setMin(0);
  streamSpeedScrollbar->setLargeStep(5);
  streamSpeedScrollbar->setSmallStep(1);
  streamSpeedScrollbar->setToolTipText(language->translate("streamSpeed").c_str());

  streamOverride = guienv->addCheckBox(false, irr::core::rect<irr::s32>(0.29*su,0.01*sh,0.31*su,0.03*sh),extraControlsTabWeather,GUI_ID_STREAMOVERRIDE_BOX);
  streamOverride->setToolTipText(language->translate("streamOverride").c_str());

  //Add buttons to control rudder failures etc.
  irr::gui::IGUITab* extraControlsTabRudder = extraControlsTabControl->addTab(language->translate("rudderFailure").c_str());

  guienv->addButton(irr::core::rect<irr::s32>(0.005*su,0.01*sh,0.16*su,0.04*sh),extraControlsTabRudder,GUI_ID_RUDDERPUMP_1_WORKING_BUTTON,language->translate("pump1Working").c_str());
  guienv->addButton(irr::core::rect<irr::s32>(0.005*su,0.04*sh,0.16*su,0.07*sh),extraControlsTabRudder,GUI_ID_RUDDERPUMP_1_FAILED_BUTTON,language->translate("pump1Failed").c_str());
  guienv->addButton(irr::core::rect<irr::s32>(0.005*su,0.08*sh,0.16*su,0.11*sh),extraControlsTabRudder,GUI_ID_RUDDERPUMP_2_WORKING_BUTTON,language->translate("pump2Working").c_str());
  guienv->addButton(irr::core::rect<irr::s32>(0.005*su,0.11*sh,0.16*su,0.14*sh),extraControlsTabRudder,GUI_ID_RUDDERPUMP_2_FAILED_BUTTON,language->translate("pump2Failed").c_str());

  guienv->addButton(irr::core::rect<irr::s32>(0.165*su,0.01*sh,0.325*su,0.04*sh),extraControlsTabRudder,GUI_ID_FOLLOWUP_WORKING_BUTTON,language->translate("followUpWorking").c_str());
  guienv->addButton(irr::core::rect<irr::s32>(0.165*su,0.04*sh,0.325*su,0.07*sh),extraControlsTabRudder,GUI_ID_FOLLOWUP_FAILED_BUTTON,language->translate("followUpFailed").c_str());

  //Add extra controls for view (zoom etc)
  irr::gui::IGUITab* extraControlsTabView = extraControlsTabControl->addTab(language->translate("view").c_str());
  guienv->addStaticText(language->translate("magnification").c_str(), irr::core::rect<irr::s32>(0.005 * su, 0.02 * sh, 0.085 * su, 0.05 * sh), false, true, extraControlsTabView)->setTextAlignment(irr::gui::EGUIA_CENTER, irr::gui::EGUIA_CENTER);
  magnificationScrollbar = new irr::gui::ScrollDial(irr::core::vector2d<irr::s32>(0.045 * su, 0.09 * sh), 0.03 * su, guienv, extraControlsTabView, GUI_ID_MAGNIFICATION_SCROLL_BAR);
  magnificationScrollbar->setMax(200); //Divide by 10 to get magnification
  magnificationScrollbar->setMin(10);
  magnificationScrollbar->setSmallStep(5);
  magnificationScrollbar->setPos(1.0 * 10); // Initialise as 1x zoom

  //Add an additional window for lines (will normally be hidden)
  // Anchored independently of stdDataDisplayPos, which now lives in the bottom row and is too short for this window.
  irr::core::rect<irr::s32> linesWindowPos = irr::core::rect<irr::s32>(0.09*su+azimuthGUIOffsetL,0.76*sh,0.45*su+azimuthGUIOffsetR,0.95*sh);
  linesWindowPos.LowerRightCorner -= irr::core::position2d<irr::s32>(0,0.03*sh);
  // Scale lines window to make smaller if possible
  irr::core::dimension2d<irr::u32> sampleDimension = guienv->getSkin()->getFont()->getDimension(L"Example");
  irr::u32 targetHeight = sampleDimension.Height * 8;
  irr::u32 targetWidth = sampleDimension.Width * 6;
  if (linesWindowPos.getHeight() > targetHeight) {
    linesWindowPos.LowerRightCorner.Y = linesWindowPos.UpperLeftCorner.Y + targetHeight;
  }
  if (linesWindowPos.getWidth() > targetWidth) {
    linesWindowPos.LowerRightCorner.X = linesWindowPos.UpperLeftCorner.X + targetWidth;
  }
  linesControlsWindow=guienv->addWindow(linesWindowPos);
  linesControlsWindow->getCloseButton()->setVisible(false);
  linesControlsWindow->setText(language->translate("lines").c_str());
  guienv->addButton(linesControlsWindow->getCloseButton()->getRelativePosition(),linesControlsWindow,GUI_ID_HIDE_LINES_CONTROLS_BUTTON,L"X");
  linesControlsWindow->setVisible(false);

  //Lines controls interface
  irr::core::rect<irr::s32> lineControlsWindowSize = linesControlsWindow->getRelativePosition();
  //irr::s32 lwVO = guienv->getSkin()->getSize(irr::gui::EGDS_WINDOW_BUTTON_WIDTH) + 2 + guienv->getSkin()->getSize(irr::gui::EGDS_MESSAGE_BOX_GAP_SPACE); // Vertical offset
  irr::s32 lwVO = guienv->getSkin() ? guienv->getSkin()->getSize(irr::gui::EGDS_WINDOW_BUTTON_WIDTH) + 5 : 20; // Vertical offset
  irr::s32 lwSu = lineControlsWindowSize.getWidth();
  irr::s32 lwSh = lineControlsWindowSize.getHeight() - lwVO;
        
  addLine = guienv->addButton(irr::core::rect<irr::s32>(0.01*lwSu, 0.01*lwSh + lwVO, 0.3*lwSu, 0.250*lwSh + lwVO),linesControlsWindow,GUI_ID_ADD_LINE_BUTTON,language->translate("addLine").c_str());
  linesList = guienv->addListBox(irr::core::rect<irr::s32>(0.01*lwSu, 0.275*lwSh + lwVO, 0.3*lwSu, 0.95*lwSh + lwVO),linesControlsWindow,GUI_ID_LINES_LIST);
        
  removeLine = guienv->addButton(irr::core::rect<irr::s32>(0.325*lwSu, 0.450*lwSh + lwVO, 0.95*lwSu,0.600*lwSh + lwVO),linesControlsWindow,GUI_ID_REMOVE_LINE_BUTTON,language->translate("removeLine").c_str());
        
  keepLineSlack = guienv->addCheckBox(false,irr::core::rect<irr::s32>(0.650*lwSu, 0.625*lwSh + lwVO, 0.725*lwSu,0.775*lwSh + lwVO),linesControlsWindow,GUI_ID_KEEP_SLACK_LINE_CHECKBOX);
  irr::gui::IGUIStaticText* keepLineSlackText = guienv->addStaticText(language->translate("keepLineSlack").c_str(),irr::core::rect<irr::s32>(0.325*lwSu, 0.625*lwSh + lwVO, 0.650*lwSu,0.775*lwSh + lwVO),true,true,linesControlsWindow);
  keepLineSlackText->setTextAlignment(irr::gui::EGUIA_LOWERRIGHT, irr::gui::EGUIA_CENTER);
        
  heaveLineIn = guienv->addCheckBox(false,irr::core::rect<irr::s32>(0.650*lwSu, 0.800*lwSh + lwVO, 0.725*lwSu,0.950*lwSh + lwVO),linesControlsWindow,GUI_ID_HAUL_IN_LINE_CHECKBOX);
  irr::gui::IGUIStaticText* haulLineInText = guienv->addStaticText(language->translate("haulLineIn").c_str(),irr::core::rect<irr::s32>(0.325*lwSu, 0.800*lwSh + lwVO, 0.650*lwSu,0.950*lwSh + lwVO),true,true,linesControlsWindow);
  haulLineInText->setTextAlignment(irr::gui::EGUIA_LOWERRIGHT, irr::gui::EGUIA_CENTER);

  anchorLine = guienv->addCheckBox(false, irr::core::rect<irr::s32>(0.875 * lwSu, 0.625 * lwSh + lwVO, 0.950 * lwSu, 0.775 * lwSh + lwVO), linesControlsWindow, GUI_ID_ANCHOR_LINE_CHECKBOX);
  irr::gui::IGUIStaticText* anchorLineText = guienv->addStaticText(language->translate("anchorLine").c_str(), irr::core::rect<irr::s32>(0.725 * lwSu, 0.625 * lwSh + lwVO, 0.875 * lwSu, 0.775 * lwSh + lwVO), true, true, linesControlsWindow);
  anchorLineText->setTextAlignment(irr::gui::EGUIA_LOWERRIGHT, irr::gui::EGUIA_CENTER);

  linesText = guienv->addStaticText(L"",irr::core::rect<irr::s32>(0.325*lwSu, 0.01*lwSh + lwVO, 0.95*lwSu, 0.425*lwSh + lwVO),true,true,linesControlsWindow);
 
  //add radar buttons
  //add tab control for radar
  radarTabControl = guienv->addTabControl(irr::core::rect<irr::s32>(0.455*su+azimuthGUIOffsetR,0.695*sh,0.697*su+azimuthGUIOffsetR,0.990*sh),0,true);
  radarTabControl->setTabHeight(0.03*sh);
  irr::gui::IGUITab* mainRadarTab = radarTabControl->addTab(language->translate("radarMainTab").c_str(),0);
  //irr::gui::IGUITab* radarEBLTab = radarTabControl->addTab(language->translate("radarEBLVRMTab").c_str(),0);
  irr::gui::IGUITab* radarPITab = radarTabControl->addTab(language->translate("radarPITab").c_str(),0);
  //irr::gui::IGUITab* radarGZoneTab = radarTabControl->addTab(language->translate("radarGuardZoneTab").c_str(),0);
  irr::gui::IGUITab* radarARPATab = radarTabControl->addTab(language->translate("radarARPATab").c_str(),0);
  //irr::gui::IGUITab* radarTrackTab = radarTabControl->addTab(language->translate("radarTrackTab").c_str(),0);
  //irr::gui::IGUITab* radarARPAVectorTab = radarTabControl->addTab(language->translate("radarARPAVectorTab").c_str(),0);
  //irr::gui::IGUITab* radarARPAAlarmTab = radarTabControl->addTab(language->translate("radarARPAAlarmTab").c_str(),0);
  //irr::gui::IGUITab* radarARPATrialTab = radarTabControl->addTab(language->translate("radarARPATrialTab").c_str(),0);

  radarText = guienv->addStaticText(L"",irr::core::rect<irr::s32>(0.460*su+azimuthGUIOffsetR,0.610*sh,0.690*su+azimuthGUIOffsetR,0.690*sh),true,true,0,-1,true);

  //Buttons for radar on/off
  radarOnOffButton = guienv->addButton(irr::core::rect<irr::s32>(0.005*su, 0.010*sh, 0.055*su, 0.040*sh), mainRadarTab, GUI_ID_RADAR_ONOFF_BUTTON, language->translate("onoff").c_str());
  //TODO: Complete this: To go where radar zoom + is, and squash these down a bit

  //Buttons for full or small radar
  bigRadarButton = guienv->addButton(irr::core::rect<irr::s32>(0.700*su+azimuthGUIOffsetR,0.610*sh,0.720*su+azimuthGUIOffsetR,0.640*sh),0,GUI_ID_BIG_RADAR_BUTTON,language->translate("bigRadar").c_str());
  irr::s32 smallRadarButtonLeft = radarTL.X + 0.01*su;
  irr::s32 smallRadarButtonTop = radarTL.Y + 0.01*sh;
  smallRadarButton = guienv->addButton(irr::core::rect<irr::s32>(smallRadarButtonLeft,smallRadarButtonTop,smallRadarButtonLeft+0.020*su,smallRadarButtonTop+0.030*sh),0,GUI_ID_SMALL_RADAR_BUTTON,language->translate("smallRadar").c_str());
  bigRadarButton->setToolTipText(language->translate("fullScreenRadar").c_str());
  smallRadarButton->setToolTipText(language->translate("minimiseRadar").c_str());

  // Radar cursor buttons
  radarCursorLeftButton = guienv->addButton(irr::core::rect<irr::s32>(0.700*su+azimuthGUIOffsetR,0.950*sh,0.715*su+azimuthGUIOffsetR,0.970*sh),0,GUI_ID_RADAR_DECREASE_X_BUTTON,L"<");
  radarCursorRightButton = guienv->addButton(irr::core::rect<irr::s32>(0.730*su+azimuthGUIOffsetR,0.950*sh,0.745*su+azimuthGUIOffsetR,0.970*sh),0,GUI_ID_RADAR_INCREASE_X_BUTTON,L">");
  radarCursorUpButton = guienv->addButton(irr::core::rect<irr::s32>(0.715*su+azimuthGUIOffsetR,0.930*sh,0.730*su+azimuthGUIOffsetR,0.950*sh),0,GUI_ID_RADAR_INCREASE_Y_BUTTON,L"^");
  radarCursorDownButton = guienv->addButton(irr::core::rect<irr::s32>(0.715*su+azimuthGUIOffsetR,0.970*sh,0.730*su+azimuthGUIOffsetR,0.990*sh),0,GUI_ID_RADAR_DECREASE_Y_BUTTON,L"v");

  guienv->addButton(irr::core::rect<irr::s32>(0.005*su,0.045*sh,0.055*su,0.085*sh),mainRadarTab,GUI_ID_RADAR_INCREASE_BUTTON,language->translate("increaserange").c_str());
  guienv->addButton(irr::core::rect<irr::s32>(0.005*su,0.085*sh,0.055*su,0.125*sh),mainRadarTab,GUI_ID_RADAR_DECREASE_BUTTON,language->translate("decreaserange").c_str());

  guienv->addButton(irr::core::rect<irr::s32>(0.005*su,0.130*sh,0.055*su,0.160*sh),mainRadarTab,GUI_ID_RADAR_NORTH_BUTTON,language->translate("northUp").c_str());
  guienv->addButton(irr::core::rect<irr::s32>(0.005*su,0.160*sh,0.055*su,0.190*sh),mainRadarTab,GUI_ID_RADAR_COURSE_BUTTON,language->translate("courseUp").c_str());
  guienv->addButton(irr::core::rect<irr::s32>(0.005*su,0.190*sh,0.055*su,0.220*sh),mainRadarTab,GUI_ID_RADAR_HEAD_BUTTON,language->translate("headUp").c_str());

  //Controls for small radar window
  radarGainScrollbar    = new irr::gui::ScrollDial(irr::core::vector2d<irr::s32>(0.0850*su,0.040*sh),0.02*su,guienv,mainRadarTab,GUI_ID_RADAR_GAIN_SCROLL_BAR);
  radarClutterScrollbar = new irr::gui::ScrollDial(irr::core::vector2d<irr::s32>(0.1425*su,0.040*sh),0.02*su,guienv,mainRadarTab,GUI_ID_RADAR_CLUTTER_SCROLL_BAR);
  radarRainScrollbar    = new irr::gui::ScrollDial(irr::core::vector2d<irr::s32>(0.2000*su,0.040*sh),0.02*su,guienv,mainRadarTab,GUI_ID_RADAR_RAIN_SCROLL_BAR);
  (guienv->addStaticText(language->translate("gain").c_str(),irr::core::rect<irr::s32>(0.0600*su,0.070*sh,0.1100*su,0.100*sh),false,true,mainRadarTab))->setTextAlignment(irr::gui::EGUIA_CENTER,irr::gui::EGUIA_CENTER);
  (guienv->addStaticText(language->translate("clutter").c_str(),irr::core::rect<irr::s32>(0.1165*su,0.070*sh,0.1675*su,0.100*sh),false,true,mainRadarTab))->setTextAlignment(irr::gui::EGUIA_CENTER,irr::gui::EGUIA_CENTER);
  (guienv->addStaticText(language->translate("rain").c_str(),irr::core::rect<irr::s32>(0.1750*su,0.070*sh,0.2250*su,0.100*sh),false,true,mainRadarTab))->setTextAlignment(irr::gui::EGUIA_CENTER,irr::gui::EGUIA_CENTER);
  radarGainScrollbar->setSmallStep(2);
  radarClutterScrollbar->setSmallStep(2);
  radarRainScrollbar->setSmallStep(2);

  eblLeftButton = guienv->addButton(irr::core::rect<irr::s32>(0.060*su,0.160*sh,0.115*su,0.190*sh),mainRadarTab,GUI_ID_RADAR_EBL_LEFT_BUTTON,language->translate("eblLeft").c_str());
  eblRightButton = guienv->addButton(irr::core::rect<irr::s32>(0.170*su,0.160*sh,0.225*su,0.190*sh),mainRadarTab,GUI_ID_RADAR_EBL_RIGHT_BUTTON,language->translate("eblRight").c_str());
  eblUpButton = guienv->addButton(irr::core::rect<irr::s32>(0.115*su,0.130*sh,0.170*su,0.160*sh),mainRadarTab,GUI_ID_RADAR_EBL_UP_BUTTON,language->translate("eblUp").c_str());
  eblDownButton = guienv->addButton(irr::core::rect<irr::s32>(0.115*su,0.190*sh,0.170*su,0.220*sh),mainRadarTab,GUI_ID_RADAR_EBL_DOWN_BUTTON,language->translate("eblDown").c_str());

  radarColourButton = guienv->addButton(irr::core::rect<irr::s32>(0.115*su,0.160*sh,0.170*su,0.190*sh),mainRadarTab,GUI_ID_RADAR_COLOUR_BUTTON,language->translate("radarColour").c_str());

  //Controls for large radar window
  largeRadarControls = new irr::gui::IGUIRectangle(guienv,guienv->getRootGUIElement(),irr::core::rect<irr::s32>(radarTL.X+0.770*radarSu,radarTL.Y+0.020*radarSu,radarTL.X+0.980*radarSu,radarTL.Y+0.730*radarSu));
  largeRadarPIControls = new irr::gui::IGUIRectangle(guienv,guienv->getRootGUIElement(),irr::core::rect<irr::s32>(radarTL.X+0.550*radarSu,radarTL.Y+0.020*radarSu,radarTL.X+0.770*radarSu,radarTL.Y+0.200*radarSu),false);
  radarGainScrollbar2    = new irr::gui::ScrollDial(irr::core::vector2d<irr::s32>(0.040*radarSu,0.040*radarSu),0.03*radarSu,guienv,largeRadarControls,GUI_ID_RADAR_GAIN_SCROLL_BAR);
  radarClutterScrollbar2 = new irr::gui::ScrollDial(irr::core::vector2d<irr::s32>(0.105*radarSu,0.040*radarSu),0.03*radarSu,guienv,largeRadarControls,GUI_ID_RADAR_CLUTTER_SCROLL_BAR);
  radarRainScrollbar2    = new irr::gui::ScrollDial(irr::core::vector2d<irr::s32>(0.170*radarSu,0.040*radarSu),0.03*radarSu,guienv,largeRadarControls,GUI_ID_RADAR_RAIN_SCROLL_BAR);

  radarGainScrollbar2->setSmallStep(2);
  radarClutterScrollbar2->setSmallStep(2);
  radarRainScrollbar2->setSmallStep(2);

  (guienv->addStaticText(language->translate("gain").c_str(),irr::core::rect<irr::s32>(0.010*radarSu,0.070*radarSu,0.070*radarSu,0.100*radarSu),false,true,largeRadarControls))->setTextAlignment(irr::gui::EGUIA_CENTER,irr::gui::EGUIA_CENTER);
  (guienv->addStaticText(language->translate("clutter").c_str(),irr::core::rect<irr::s32>(0.075*radarSu,0.070*radarSu,0.135*radarSu,0.100*radarSu),false,true,largeRadarControls))->setTextAlignment(irr::gui::EGUIA_CENTER,irr::gui::EGUIA_CENTER);
  (guienv->addStaticText(language->translate("rain").c_str(),irr::core::rect<irr::s32>(0.140*radarSu,0.070*radarSu,0.200*radarSu,0.100*radarSu),false,true,largeRadarControls))->setTextAlignment(irr::gui::EGUIA_CENTER,irr::gui::EGUIA_CENTER);

  guienv->addButton(irr::core::rect<irr::s32>(0.025*radarSu,0.110*radarSu,0.085*radarSu,0.160*radarSu),largeRadarControls,GUI_ID_RADAR_INCREASE_BUTTON,language->translate("increaserange").c_str());
  guienv->addButton(irr::core::rect<irr::s32>(0.025*radarSu,0.165*radarSu,0.085*radarSu,0.210*radarSu),largeRadarControls,GUI_ID_RADAR_DECREASE_BUTTON,language->translate("decreaserange").c_str());

  guienv->addButton(irr::core::rect<irr::s32>(0.125*radarSu,0.110*radarSu,0.190*radarSu,0.140*radarSu),largeRadarControls,GUI_ID_RADAR_NORTH_BUTTON,language->translate("northUp").c_str());
  guienv->addButton(irr::core::rect<irr::s32>(0.125*radarSu,0.145*radarSu,0.190*radarSu,0.175*radarSu),largeRadarControls,GUI_ID_RADAR_COURSE_BUTTON,language->translate("courseUp").c_str());
  guienv->addButton(irr::core::rect<irr::s32>(0.125*radarSu,0.180*radarSu,0.190*radarSu,0.210*radarSu),largeRadarControls,GUI_ID_RADAR_HEAD_BUTTON,language->translate("headUp").c_str());

  eblLeftButton2 = guienv->addButton(irr::core::rect<irr::s32>(0.025*radarSu,0.245*radarSu,0.080*radarSu,0.275*radarSu),largeRadarControls,GUI_ID_RADAR_EBL_LEFT_BUTTON,language->translate("eblLeft").c_str());
  eblRightButton2 = guienv->addButton(irr::core::rect<irr::s32>(0.135*radarSu,0.245*radarSu,0.190*radarSu,0.275*radarSu),largeRadarControls,GUI_ID_RADAR_EBL_RIGHT_BUTTON,language->translate("eblRight").c_str());
  eblUpButton2 = guienv->addButton(irr::core::rect<irr::s32>(0.080*radarSu,0.215*radarSu,0.135*radarSu,0.245*radarSu),largeRadarControls,GUI_ID_RADAR_EBL_UP_BUTTON,language->translate("eblUp").c_str());
  eblDownButton2 = guienv->addButton(irr::core::rect<irr::s32>(0.080*radarSu,0.275*radarSu,0.135*radarSu,0.305*radarSu),largeRadarControls,GUI_ID_RADAR_EBL_DOWN_BUTTON,language->translate("eblDown").c_str());

  radarColourButton2 = guienv->addButton(irr::core::rect<irr::s32>(0.080*radarSu,0.245*radarSu,0.135*radarSu,0.275*radarSu),largeRadarControls,GUI_ID_RADAR_COLOUR_BUTTON,language->translate("radarColour").c_str());

  // Radar cursor buttons
  radarCursorLeftButton2 = guienv->addButton(irr::core::rect<irr::s32>(radarTL.X+0.670*radarSu,radarTL.Y+0.640*radarSu,radarTL.X+0.700*radarSu,radarTL.Y+0.670*radarSu),0,GUI_ID_RADAR_DECREASE_X_BUTTON,L"<");
  radarCursorRightButton2 = guienv->addButton(irr::core::rect<irr::s32>(radarTL.X+0.730*radarSu,radarTL.Y+0.640*radarSu,radarTL.X+0.760*radarSu,radarTL.Y+0.670*radarSu),0,GUI_ID_RADAR_INCREASE_X_BUTTON,L">");
  radarCursorUpButton2 = guienv->addButton(irr::core::rect<irr::s32>(radarTL.X+0.700*radarSu,radarTL.Y+0.610*radarSu,radarTL.X+0.730*radarSu,radarTL.Y+0.640*radarSu),0,GUI_ID_RADAR_INCREASE_Y_BUTTON,L"^");
  radarCursorDownButton2 = guienv->addButton(irr::core::rect<irr::s32>(radarTL.X+0.700*radarSu,radarTL.Y+0.670*radarSu,radarTL.X+0.730*radarSu,radarTL.Y+0.700*radarSu),0,GUI_ID_RADAR_DECREASE_Y_BUTTON,L"v");

  radarText2 = guienv->addStaticText(L"",irr::core::rect<irr::s32>(0.010*radarSu,0.310*radarSu,0.200*radarSu,0.400*radarSu),true,true,largeRadarControls,-1,true);

  //Radar PI tab
  //Drop down box to select PI 1-10
  (guienv->addStaticText(language->translate("parallelIndex").c_str(),irr::core::rect<irr::s32>(0.055*su,0.040*sh,0.205*su,0.080*sh),false,true,radarPITab))->setTextAlignment(irr::gui::EGUIA_UPPERLEFT,irr::gui::EGUIA_CENTER);
  irr::gui::IGUIComboBox* piSelected = guienv->addComboBox(irr::core::rect<irr::s32>(0.005*su,0.040*sh,0.050*su,0.080*sh),radarPITab,GUI_ID_PI_SELECT_BOX);
  piSelected->addItem(L"1");
  piSelected->addItem(L"2");
  piSelected->addItem(L"3");
  piSelected->addItem(L"4");
  piSelected->addItem(L"5");
  piSelected->addItem(L"6");
  piSelected->addItem(L"7");
  piSelected->addItem(L"8");
  piSelected->addItem(L"9");
  piSelected->addItem(L"10");
  //Edit boxes for bearing and range (+ve/-ve)
  (guienv->addStaticText(language->translate("piRange").c_str(),irr::core::rect<irr::s32>(0.055*su,0.100*sh,0.205*su,0.140*sh),false,true,radarPITab))->setTextAlignment(irr::gui::EGUIA_UPPERLEFT,irr::gui::EGUIA_CENTER);;
  guienv->addEditBox(L"0",irr::core::rect<irr::s32>(0.005*su,0.100*sh,0.050*su,0.140*sh),true,radarPITab,GUI_ID_PI_RANGE_BOX);
  (guienv->addStaticText(language->translate("piBearing").c_str(),irr::core::rect<irr::s32>(0.055*su,0.160*sh,0.205*su,0.200*sh),false,true,radarPITab))->setTextAlignment(irr::gui::EGUIA_UPPERLEFT,irr::gui::EGUIA_CENTER);;
  guienv->addEditBox(L"0",irr::core::rect<irr::s32>(0.005*su,0.160*sh,0.050*su,0.200*sh),true,radarPITab,GUI_ID_PI_BEARING_BOX);

  //PI on big radar screen
  (guienv->addStaticText(language->translate("parallelIndex").c_str(),irr::core::rect<irr::s32>(0.005*radarSu,0.010*radarSu,0.075*radarSu,0.070*radarSu),false,true,largeRadarPIControls))->setTextAlignment(irr::gui::EGUIA_LOWERRIGHT,irr::gui::EGUIA_UPPERLEFT);
  irr::gui::IGUIComboBox* piSelectedBig = guienv->addComboBox(irr::core::rect<irr::s32>(0.080*radarSu,0.010*radarSu,0.195*radarSu,0.035*radarSu),largeRadarPIControls,GUI_ID_BIG_PI_SELECT_BOX);
  piSelectedBig->addItem(L"1");
  piSelectedBig->addItem(L"2");
  piSelectedBig->addItem(L"3");
  piSelectedBig->addItem(L"4");
  piSelectedBig->addItem(L"5");
  piSelectedBig->addItem(L"6");
  piSelectedBig->addItem(L"7");
  piSelectedBig->addItem(L"8");
  piSelectedBig->addItem(L"9");
  piSelectedBig->addItem(L"10");

  guienv->addStaticText(language->translate("PIrange").c_str(),irr::core::rect<irr::s32>(0.130*radarSu,0.045*radarSu,0.215*radarSu,0.070*radarSu),false,false,largeRadarPIControls);
  guienv->addEditBox(L"0",irr::core::rect<irr::s32>(0.080*radarSu,0.045*radarSu,0.125*radarSu,0.070*radarSu),true,largeRadarPIControls,GUI_ID_BIG_PI_RANGE_BOX);

  guienv->addStaticText(language->translate("PIbearing").c_str(),irr::core::rect<irr::s32>(0.130*radarSu,0.080*radarSu,0.215*radarSu,0.105*radarSu),false,false,largeRadarPIControls);
  guienv->addEditBox(L"0",irr::core::rect<irr::s32>(0.080*radarSu,0.080*radarSu,0.125*radarSu,0.105*radarSu),true,largeRadarPIControls,GUI_ID_BIG_PI_BEARING_BOX);

  //Radar ARPA tab
  irr::gui::IGUIComboBox* arpaMode = guienv->addComboBox(irr::core::rect<irr::s32>(0.005*su,0.005*sh,0.150*su,0.035*sh),radarARPATab,GUI_ID_ARPA_ON_BOX);
  arpaMode->addItem(language->translate("arpaManual").c_str());
  arpaMode->addItem(language->translate("marpaOn").c_str());
  arpaMode->addItem(language->translate("arpaOn").c_str());
  irr::gui::IGUIComboBox* arpaVectorMode = guienv->addComboBox(irr::core::rect<irr::s32>(0.005*su,0.040*sh,0.150*su,0.070*sh),radarARPATab,GUI_ID_ARPA_TRUE_REL_BOX);
  arpaVectorMode->addItem(language->translate("trueArpa").c_str());
  arpaVectorMode->addItem(language->translate("relArpa").c_str());
  guienv->addEditBox(L"6",irr::core::rect<irr::s32>(0.155*su,0.040*sh,0.195*su,0.070*sh),true,radarARPATab,GUI_ID_ARPA_VECTOR_TIME_BOX);
  (guienv->addStaticText(language->translate("minsARPA").c_str(),irr::core::rect<irr::s32>(0.200*su,0.040*sh,0.237*su,0.070*sh),false,true,radarARPATab))->setTextAlignment(irr::gui::EGUIA_CENTER,irr::gui::EGUIA_CENTER);
  arpaList = guienv->addListBox(irr::core::rect<irr::s32>(0.005*su,0.075*sh,0.121*su,0.190*sh),radarARPATab,GUI_ID_ARPA_LIST);
  arpaText = guienv->addListBox(irr::core::rect<irr::s32>(0.121*su,0.075*sh,0.237*su,0.190*sh),radarARPATab);
  // Manual/MARPA buttons
  (guienv->addStaticText(language->translate("manualOrMarpa").c_str(), irr::core::rect<irr::s32>(0.005*su,0.190*sh,0.237*su,0.215*sh), false, true, radarARPATab))->setTextAlignment(irr::gui::EGUIA_CENTER, irr::gui::EGUIA_CENTER);
  guienv->addButton(irr::core::rect<irr::s32>(0.005*su,0.215*sh,0.082*su,0.240*sh),radarARPATab,GUI_ID_MANUAL_NEW_BUTTON,language->translate("new").c_str());
  guienv->addButton(irr::core::rect<irr::s32>(0.082*su,0.215*sh,0.159*su,0.240*sh),radarARPATab,GUI_ID_MANUAL_SCAN_BUTTON,language->translate("manualLog").c_str());
  guienv->addButton(irr::core::rect<irr::s32>(0.159*su,0.215*sh,0.237*su,0.240*sh),radarARPATab,GUI_ID_MANUAL_CLEAR_BUTTON,language->translate("clear").c_str());

  //Radar ARPA on big radar screen
  arpaMode = guienv->addComboBox(irr::core::rect<irr::s32>(0.010*radarSu,0.405*radarSu,0.200*radarSu,0.435*radarSu),largeRadarControls,GUI_ID_BIG_ARPA_ON_BOX);
  arpaMode->addItem(language->translate("arpaManual").c_str());
  arpaMode->addItem(language->translate("marpaOn").c_str());
  arpaMode->addItem(language->translate("arpaOn").c_str());
  arpaVectorMode = guienv->addComboBox(irr::core::rect<irr::s32>(0.010*radarSu,0.440*radarSu,0.200*radarSu,0.470*radarSu),largeRadarControls,GUI_ID_BIG_ARPA_TRUE_REL_BOX);
  arpaVectorMode->addItem(language->translate("trueArpa").c_str());
  arpaVectorMode->addItem(language->translate("relArpa").c_str());
  guienv->addEditBox(L"6",irr::core::rect<irr::s32>(0.010*radarSu,0.480*radarSu,0.050*radarSu,0.510*radarSu),true,largeRadarControls,GUI_ID_BIG_ARPA_VECTOR_TIME_BOX);
  (guienv->addStaticText(language->translate("minsARPA").c_str(),irr::core::rect<irr::s32>(0.060*radarSu,0.480*radarSu,0.105*radarSu,0.510*radarSu),false,true,largeRadarControls))->setTextAlignment(irr::gui::EGUIA_CENTER,irr::gui::EGUIA_CENTER);
  arpaList2 = guienv->addListBox(irr::core::rect<irr::s32>(0.010*radarSu,0.515*radarSu,0.105*radarSu,0.655*radarSu),largeRadarControls,GUI_ID_BIG_ARPA_LIST);
  arpaText2 = guienv->addListBox(irr::core::rect<irr::s32>(0.105*radarSu,0.515*radarSu,0.200*radarSu,0.655*radarSu),largeRadarControls);
  // Manual/MARPA buttons
  (guienv->addStaticText(language->translate("manualOrMarpa").c_str(), irr::core::rect<irr::s32>(0.010*radarSu,0.655*radarSu,0.200*radarSu,0.675*radarSu), false, true, largeRadarControls))->setTextAlignment(irr::gui::EGUIA_CENTER, irr::gui::EGUIA_CENTER);
  guienv->addButton(irr::core::rect<irr::s32>(0.010*radarSu,0.675*radarSu,0.073*radarSu,0.695*radarSu),largeRadarControls,GUI_ID_MANUAL_NEW_BUTTON,language->translate("new").c_str());
  guienv->addButton(irr::core::rect<irr::s32>(0.073*radarSu,0.675*radarSu,0.136*radarSu,0.695*radarSu),largeRadarControls,GUI_ID_MANUAL_SCAN_BUTTON,language->translate("manualLog").c_str());
  guienv->addButton(irr::core::rect<irr::s32>(0.136*radarSu,0.675*radarSu,0.200*radarSu,0.695*radarSu),largeRadarControls,GUI_ID_MANUAL_CLEAR_BUTTON,language->translate("clear").c_str());


  //Add paused button
  irr::core::stringw pausedButtonMessage = language->translate("pausedbutton");
  if (vr3dMode) {
    pausedButtonMessage = pausedButtonMessage + language->translate("vrpausedbutton");
  } else {
    pausedButtonMessage = pausedButtonMessage + language->translate("normalpausedbutton");
  }
  pausedButton = guienv->addButton(irr::core::rect<irr::s32>(0.2*su,0.1*sh,0.8*su,0.9*sh),0,GUI_ID_START_BUTTON, pausedButtonMessage.c_str());

  //show/hide interface
  showInterface = true; //If we start with the 2d interface shown
  showInterfaceButton = guienv->addButton(irr::core::rect<irr::s32>(0.09*su+azimuthGUIOffsetL,buttonRowTop*sh,0.125*su+azimuthGUIOffsetL,buttonRowBottom*sh),0,GUI_ID_SHOW_INTERFACE_BUTTON,language->translate("showinterface").c_str());
  hideInterfaceButton = guienv->addButton(irr::core::rect<irr::s32>(0.09*su+azimuthGUIOffsetL,buttonRowTop*sh,0.125*su+azimuthGUIOffsetL,buttonRowBottom*sh),0,GUI_ID_HIDE_INTERFACE_BUTTON,language->translate("hideinterface").c_str());
  showInterfaceButton->setVisible(false);

  //binoculars button
  binosButton = guienv->addButton(irr::core::rect<irr::s32>(0.125*su+azimuthGUIOffsetL,buttonRowTop*sh,0.16*su+azimuthGUIOffsetL,buttonRowBottom*sh),0,GUI_ID_BINOS_INTERFACE_BUTTON,language->translate("zoom").c_str());
  binosButton->setIsPushButton(true);

  //Take bearing button
  bearingButton = guienv->addButton(irr::core::rect<irr::s32>(0.16*su+azimuthGUIOffsetL,buttonRowTop*sh,0.195*su+azimuthGUIOffsetL,buttonRowBottom*sh),0,GUI_ID_BEARING_INTERFACE_BUTTON,language->translate("bearing").c_str());
  bearingButton->setIsPushButton(true);

  // Change view button
  changeViewButton = guienv->addButton(irr::core::rect<irr::s32>(0.195*su+azimuthGUIOffsetL,buttonRowTop*sh,0.23*su+azimuthGUIOffsetL,buttonRowBottom*sh),0,GUI_ID_CHANGE_VIEW_BUTTON,language->translate("changeView").c_str());

  //Exit button
  exitButton = guienv->addButton(irr::core::rect<irr::s32>(0.23*su+azimuthGUIOffsetL,buttonRowTop*sh,0.265*su+azimuthGUIOffsetL,buttonRowBottom*sh),0,GUI_ID_EXIT_BUTTON,language->translate("exit").c_str());

  //Show button to display extra controls window
  showExtraControlsButton = guienv->addButton(irr::core::rect<irr::s32>(0.265*su+azimuthGUIOffsetL,buttonRowTop*sh,0.34*su+azimuthGUIOffsetL,buttonRowBottom*sh),0,GUI_ID_SHOW_EXTRA_CONTROLS_BUTTON,language->translate("extraControls").c_str());

  //Show button to display lines control window
  showLinesControlsButton = guienv->addButton(irr::core::rect<irr::s32>(0.34*su+azimuthGUIOffsetL,buttonRowTop*sh,0.375*su+azimuthGUIOffsetL,buttonRowBottom*sh),0,GUI_ID_SHOW_LINES_CONTROLS_BUTTON,language->translate("lines").c_str());

  //Show internal log window button
  pcLogButton = guienv->addButton(irr::core::rect<irr::s32>(0.375*su+azimuthGUIOffsetL,buttonRowTop*sh,0.39*su+azimuthGUIOffsetL,buttonRowBottom*sh),0,GUI_ID_SHOW_LOG_BUTTON,language->translate("log").c_str());
        
  //Set initial visibility
  updateVisibility();

}

GUIMain::~GUIMain()
{
  //Drop scroll bars created with 'new'
  if (portScrollbar) {portScrollbar->drop();}
  if (stbdScrollbar) {stbdScrollbar->drop();}
  if (wheelScrollbar) {wheelScrollbar->drop();}

  if (rateofturnScrollbar) {rateofturnScrollbar->drop();}

  if (bowThrusterScrollbar) {bowThrusterScrollbar->drop();}
  if (sternThrusterScrollbar) {sternThrusterScrollbar->drop();}

  weatherScrollbar->drop();
  visibilityScrollbar->drop();
  rainScrollbar->drop();
  windDirectionScrollbar->drop();
  windSpeedScrollbar->drop();
  streamDirectionScrollbar->drop();
  streamSpeedScrollbar->drop();

  radarGainScrollbar->drop();
  radarClutterScrollbar->drop();
  radarRainScrollbar->drop();

  radarGainScrollbar2->drop();
  radarClutterScrollbar2->drop();
  radarRainScrollbar2->drop();

  //largeRadarControls->drop();

  hdgScrollbar->drop();
  spdScrollbar->drop();

  headingIndicator->drop();

  magnificationScrollbar->drop();
}

bool GUIMain::getShowInterface() const
{
  return showInterface;
}

// Roll round between normal view, full view with limited gui, and full view with no GUI
void GUIMain::toggleShow2dInterface()
{
  if (!getLargeRadar()) {
    if (!showInterface) {
      if (guienv->getRootGUIElement()->isVisible()) {
	guienv->getRootGUIElement()->setVisible(false);
      } else {
	showInterface = true;
	guienv->getRootGUIElement()->setVisible(true);
      }

    } else {
      showInterface = false;
    }
    updateVisibility();
  }
}

void GUIMain::show2dInterface()
{
  showInterface = true;
  updateVisibility();
}

void GUIMain::hide2dInterface()
{
  showInterface = false;
  updateVisibility();
}

void GUIMain::hide2dInterfaceFull()
{
  showInterface = false;
  updateVisibility(true);
}


void GUIMain::showBearings()
{
  bearingButton->setPressed(true);
}

void GUIMain::hideBearings()
{
  bearingButton->setPressed(false);
}

void GUIMain::toggleBearings()
{
  bearingButton->setPressed(!bearingButton->isPressed());
}

void GUIMain::zoomOn()
{
  binosButton->setPressed(true);
}

void GUIMain::zoomOff()
{
  binosButton->setPressed(false);
}

void GUIMain::setSingleEngine()
{
  singleEngine = true;
}
void GUIMain::setLargeRadar(bool radarState)
{
  radarLarge = radarState;
  updateVisibility();
}

bool GUIMain::getLargeRadar() const
{
  return radarLarge;
}

bool GUIMain::getAnchorLine() const
{
  return anchorLine->isChecked();
}

void GUIMain::setARPAComboboxes(irr::s32 arpaState)
{
  //Set both linked inputs - brute force
  irr::gui::IGUIElement* arpaCheckbox = device->getGUIEnvironment()->getRootGUIElement()->getElementFromId(GUIMain::GUI_ID_ARPA_ON_BOX,true);
  if(arpaCheckbox!=0) {
    ((irr::gui::IGUIComboBox*)arpaCheckbox)->setSelected(arpaState);
  }
  arpaCheckbox = device->getGUIEnvironment()->getRootGUIElement()->getElementFromId(GUIMain::GUI_ID_BIG_ARPA_ON_BOX,true);
  if(arpaCheckbox!=0) {
    ((irr::gui::IGUIComboBox*)arpaCheckbox)->setSelected(arpaState);
  }
}

void GUIMain::setARPAList(int arpaSelected)
{
  //Set both linked inputs - brute force
  if(arpaList!=0) {
    arpaList->setSelected(arpaSelected);
  }
        
  if(arpaList2!=0) {
    arpaList2->setSelected(arpaSelected);
  }
}

irr::u32 GUIMain::getRadarPixelRadius() const
{
  if (radarLarge) {
    return largeRadarScreenRadius;
  } else {
    return smallRadarScreenRadius;
  }
}

irr::core::vector2di GUIMain::getCursorPositionRadar() const
{
  //Basic mouse position
  irr::core::vector2di cursorPosition = device->getCursorControl()->getPosition();

  //Radar screen centre position
  irr::core::vector2di radarScreenCentre;
  if (radarLarge) {
    radarScreenCentre.X = largeRadarScreenCentreX;
    radarScreenCentre.Y = largeRadarScreenCentreY;
  } else {
    radarScreenCentre.X = smallRadarScreenCentreX;
    radarScreenCentre.Y = smallRadarScreenCentreY;
  }

  //Return the difference
  return (cursorPosition-radarScreenCentre);
}

irr::core::rect<irr::s32> GUIMain::getSmallRadarRect() const
{
  irr::u32 graphicsWidth3d = su;
  irr::u32 graphicsHeight3d = sh * VIEW_PROPORTION_3D;
  return irr::core::rect<irr::s32>(su-(sh-graphicsHeight3d)+azimuthGUIOffsetR,graphicsHeight3d,su+azimuthGUIOffsetR,sh);
}
    
irr::core::rect<irr::s32> GUIMain::getLargeRadarRect() const
{
  return irr::core::rect<irr::s32>(largeRadarScreenCentreX - largeRadarScreenRadius, largeRadarScreenCentreY - largeRadarScreenRadius, largeRadarScreenCentreX + largeRadarScreenRadius, largeRadarScreenCentreY + largeRadarScreenRadius);
}

bool GUIMain::isNFUActive() const
{
  return (nfuPortDown || nfuStbdDown);
}


void GUIMain::updateVisibility(bool bHideFull)
{
  //Items to show if we're showing interface
  if (radarTabControl) { radarTabControl->setVisible(showInterface); }
  if (radarText) { radarText->setVisible(showInterface); }

  if (radarCursorLeftButton) { radarCursorLeftButton->setVisible(showInterface && !radarLarge); }
  if (radarCursorRightButton) { radarCursorRightButton->setVisible(showInterface && !radarLarge); }
  if (radarCursorUpButton) { radarCursorUpButton->setVisible(showInterface && !radarLarge); }
  if (radarCursorDownButton) { radarCursorDownButton->setVisible(showInterface && !radarLarge); }

  if (radarCursorLeftButton2) { radarCursorLeftButton2->setVisible(radarLarge); }
  if (radarCursorRightButton2) { radarCursorRightButton2->setVisible(radarLarge); }
  if (radarCursorUpButton2) { radarCursorUpButton2->setVisible(radarLarge); }
  if (radarCursorDownButton2) { radarCursorDownButton2->setVisible(radarLarge); }

  //weatherScrollbar->setVisible(showInterface);
  //rainScrollbar->setVisible(showInterface);
  //visibilityScrollbar->setVisible(showInterface);
  if (pcLogButton) { pcLogButton->setVisible(showInterface); }
  if (showExtraControlsButton) { showExtraControlsButton->setVisible(showInterface); }
  if (showLinesControlsButton) { showLinesControlsButton->setVisible(showInterface); }

  if (exitButton) { exitButton->setVisible(showInterface); }

  if (portText) {portText->setVisible(showInterface);}
  if (stbdText) {stbdText->setVisible(showInterface && !singleEngine);}


  //Items not to show if we're on full screen radar
  //dataDisplay->setVisible(!radarLarge);
  if (binosButton) { binosButton->setVisible(!radarLarge); }
  if (bearingButton) { bearingButton->setVisible(!radarLarge); }
  if (changeViewButton) { changeViewButton->setVisible(!radarLarge); }
  if (compassBG) { compassBG->setVisible(showInterface && !radarLarge); }
  if (compassLabel) { compassLabel->setVisible(showInterface && !radarLarge); }
  if (rateofturnScrollbar) { rateofturnScrollbar->setVisible(showInterface && !radarLarge && hasRateOfTurnIndicator); }
  if (rateofturnText) { rateofturnText->setVisible(showInterface && !radarLarge && hasRateOfTurnIndicator); }
  if (hideInterfaceButton) { hideInterfaceButton->setVisible(showInterface && !radarLarge); }
  if (showInterfaceButton) { showInterfaceButton->setVisible(!showInterface && !radarLarge); }
        
  if (bigRadarButton) { bigRadarButton->setVisible(showInterface && !radarLarge); }

  if (smallRadarButton) { smallRadarButton->setVisible(radarLarge); }
  if (largeRadarControls) { largeRadarControls->setVisible(radarLarge); }
  if (largeRadarPIControls) { largeRadarPIControls->setVisible(radarLarge); }

  //Move gui elements if on largescreen radar
  //Heading
  if (headingIndicator) {
    if (radarLarge) {
      headingIndicator->setRelativePosition(radHdgIndicatorPos);
    } else if (!showInterface) {
      headingIndicator->setRelativePosition(maxHdgIndicatorPos);
    } else {
      headingIndicator->setRelativePosition(stdHdgIndicatorPos);
    }
  }
  //Set position of data display
  // (list boxes use the skin's background colour rather than a per-instance colour, so there's no
  // separate background swap here any more - just repositioning between the three display modes)
  if (dataDisplay) {
    if (radarLarge) {
      dataDisplay->setRelativePosition(radDataDisplayPos);
    } else if (!showInterface) {
      dataDisplay->setRelativePosition(altDataDisplayPos);
    } else {
      dataDisplay->setRelativePosition(stdDataDisplayPos);
    }
  }

  // Restore visibility of elements that hideInSecondary/bHideFull may have hidden,
  // so toggling showInterface works correctly after a hide2dInterfaceFull() call.
  if (headingIndicator) { headingIndicator->setVisible(showInterface); }
  if (dataDisplay) { dataDisplay->setVisible(true); }
  if (portScrollbar) { portScrollbar->setVisible(showInterface); }
  if (stbdScrollbar) { stbdScrollbar->setVisible(showInterface && !singleEngine); }
  if (wheelScrollbar) { wheelScrollbar->setVisible(showInterface); }
  if (wheelLabel) { wheelLabel->setVisible(showInterface); }
  if (nonFollowUpPortButton) { nonFollowUpPortButton->setVisible(showInterface); }
  if (nonFollowUpStbdButton) { nonFollowUpStbdButton->setVisible(showInterface); }
  if (bowThrusterScrollbar) { bowThrusterScrollbar->setVisible(showInterface); }
  if (bowThrusterNameLabel) { bowThrusterNameLabel->setVisible(showInterface); }
  if (bowThrusterLeftButton) { bowThrusterLeftButton->setVisible(showInterface); }
  if (bowThrusterRightButton) { bowThrusterRightButton->setVisible(showInterface); }
  if (sternThrusterScrollbar) { sternThrusterScrollbar->setVisible(showInterface); }
  if (sternThrusterNameLabel) { sternThrusterNameLabel->setVisible(showInterface); }
  if (sternThrusterLeftButton) { sternThrusterLeftButton->setVisible(showInterface); }
  if (sternThrusterRightButton) { sternThrusterRightButton->setVisible(showInterface); }

  //If we're in secondary mode, make sure things are hidden if they shouldn't be shown on the secondary screen
  if (controlsHidden || bHideFull) {
    hideInSecondary();
  }

  if(bHideFull)
    {
      if(headingIndicator) headingIndicator->setVisible(false);
      if(rateofturnScrollbar) rateofturnScrollbar->setVisible(false);
      if(rateofturnText) rateofturnText->setVisible(false);
      if(dataDisplay) dataDisplay->setVisible(false);
    }

}

void GUIMain::hideInSecondary() {
  //Hide user inputs if in secondary mode
  if (stbdScrollbar) {stbdScrollbar->setVisible(false);}
  if (portScrollbar) {portScrollbar->setVisible(false);}
  if (azimuth1Master) {azimuth1Master->setVisible(false);}
  if (azimuth2Master) {azimuth2Master->setVisible(false);}

  if (showLinesControlsButton) {showLinesControlsButton->setVisible(false);}
  if (showExtraControlsButton) {showExtraControlsButton->setVisible(false);}

  if (stbdText) {stbdText->setVisible(false);}
  if (portText) {portText->setVisible(false);}
  if (wheelScrollbar) {wheelScrollbar->setVisible(false);}
  if (wheelLabel) {wheelLabel->setVisible(false);}
  if (nonFollowUpPortButton) {nonFollowUpPortButton->setVisible(false);}
  if (nonFollowUpStbdButton) {nonFollowUpStbdButton->setVisible(false);}
  //rateofturnScrollbar->setVisible(false); // hides rate of turn indicator in full screen
  if (bowThrusterScrollbar) {bowThrusterScrollbar->setVisible(false);}
  if (bowThrusterNameLabel) {bowThrusterNameLabel->setVisible(false);}
  if (bowThrusterLeftButton) {bowThrusterLeftButton->setVisible(false);}
  if (bowThrusterRightButton) {bowThrusterRightButton->setVisible(false);}
  if (sternThrusterScrollbar) {sternThrusterScrollbar->setVisible(false);}
  if (sternThrusterNameLabel) {sternThrusterNameLabel->setVisible(false);}
  if (sternThrusterLeftButton) {sternThrusterLeftButton->setVisible(false);}
  if (sternThrusterRightButton) {sternThrusterRightButton->setVisible(false);}
}

std::wstring GUIMain::f32To1dp(irr::f32 value)
{
  //Convert a floating point value to a wstring, with 1dp
  char tempStr[100];
  snprintf(tempStr,100,"%.1f",value);
  return std::wstring(tempStr, tempStr+strlen(tempStr));
}

std::wstring GUIMain::f32To2dp(irr::f32 value)
{
  //Convert a floating point value to a wstring, with 2dp
  char tempStr[100];
  snprintf(tempStr,100,"%.2f",value);
  return std::wstring(tempStr, tempStr+strlen(tempStr));
}

std::wstring GUIMain::f32To3dp(irr::f32 value)
{
  //Convert a floating point value to a wstring, with 3dp
  char tempStr[100];
  snprintf(tempStr,100,"%.3f",value);
  return std::wstring(tempStr, tempStr+strlen(tempStr));
}

bool GUIMain::manuallyTriggerClick(irr::gui::IGUIButton* button)
{
  irr::SEvent triggerUpdateEvent;
  triggerUpdateEvent.EventType = irr::EET_GUI_EVENT;
  triggerUpdateEvent.GUIEvent.Caller = button;
  triggerUpdateEvent.GUIEvent.Element = 0;
  triggerUpdateEvent.GUIEvent.EventType = irr::gui::EGET_BUTTON_CLICKED ;
  return device->postEventFromUser(triggerUpdateEvent);
}

void GUIMain::updateGuiData(GUIData* guiData)
{

  // TODO: Check the scroll bars exist!

  //Hide the 'hint' bars
  if (device->getTimer()->getTime()>3000) {
    if (clickForEngineText) {clickForEngineText->setVisible(false);}
    if (clickForRudderText) {clickForRudderText->setVisible(false);}
  }

  //Update scroll bars
  hdgScrollbar->setPos(Utilities::round(guiData->hdg));
  spdScrollbar->setPos(Utilities::round(guiData->spd));
  if (portScrollbar) {
    //Engine units are +- 1, scale to -+100, inverted as astern is at bottom of scroll bar
    static_cast<irr::gui::OutlineScrollBar*>(portScrollbar)->setSecondary(Utilities::round(guiData->portEngActual * -100));
    portScrollbar->setPos(Utilities::round(guiData->portEng * -100));
  }
  if (stbdScrollbar) {
    static_cast<irr::gui::OutlineScrollBar*>(stbdScrollbar)->setSecondary(Utilities::round(guiData->stbdEngActual * -100));
    stbdScrollbar->setPos(Utilities::round(guiData->stbdEng * -100));
  }
  //rudderScrollbar->setPos(Utilities::round(guiData->rudder));
  if (wheelScrollbar) {
    wheelScrollbar->setSecondary(Utilities::round(guiData->rudder));
    wheelScrollbar->setPos(Utilities::round(guiData->wheel));
  }
  if (bowThrusterScrollbar) {bowThrusterScrollbar->setPos(Utilities::round(guiData->bowThruster * 100));}
  if (sternThrusterScrollbar) {sternThrusterScrollbar->setPos(Utilities::round(guiData->sternThruster * 100));}

  // TODO: is the 'round' needed here?
  radarGainScrollbar->setPos(Utilities::round(guiData->radarGain));
  radarClutterScrollbar->setPos(Utilities::round(guiData->radarClutter));
  radarRainScrollbar->setPos(Utilities::round(guiData->radarRain));

  radarGainScrollbar2->setPos(Utilities::round(guiData->radarGain));
  radarClutterScrollbar2->setPos(Utilities::round(guiData->radarClutter));
  radarRainScrollbar2->setPos(Utilities::round(guiData->radarRain));

  weatherScrollbar->setPos(Utilities::round(guiData->weather*10.0)); //(Weather scroll bar is 0-120, weather is 0-12)
  rainScrollbar->setPos(Utilities::round(guiData->rain*10.0)); //(Rain scroll bar is 0-100, rain is 0-10)
  visibilityScrollbar->setPos(Utilities::round(guiData->visibility*10.0)); //Visibility scroll bar is 1-101, visibility is 0.1 to 10.1 Nm

  windDirectionScrollbar->setPos(Utilities::round(guiData->windDirection));
  windSpeedScrollbar->setPos(Utilities::round(guiData->windSpeed));

  streamDirectionScrollbar->setPos(Utilities::round(guiData->streamDirection));
  streamSpeedScrollbar->setPos(Utilities::round(guiData->streamSpeed));
  streamOverride->setChecked(guiData->streamOverride);


  // DEE vvvvv  this should display the rate of turn data on the screen
  // DEE        since internalrate of turn is in rads per second then for deg per min x 3438
  {
    irr::f32 rotDegMin = 3438 * guiData->RateOfTurn;
    if (rateofturnScrollbar) { rateofturnScrollbar->setPos(Utilities::round(rotDegMin)); }
    if (rateofturnText) {
      std::wstring rotLabel = L"RoT: " + f32To1dp(rotDegMin) + L" °/min";
      rateofturnText->setText(rotLabel.c_str());
    }
  }
  // DEE ^^^^


  if(compassLabel)
    {
      irr::core::stringw compassTxt = language->translate("compass") + L": " + f32To1dp(guiData->hdg).c_str() + L" °";
      compassLabel->setText(compassTxt.c_str());
    }
  
  //Update text display data
  guiLat = guiData->lat;
  guiLong = guiData->longitude;
  guiHeading = guiData->hdg; //Heading in degrees
  headingIndicator->setHeading(guiHeading);
  viewHdg = guiData->viewAngle;
  viewElev = guiData->viewElevationAngle;
  while (viewHdg>=360) {viewHdg-=360;}
  while (viewHdg<0) {viewHdg+=360;}
  guiSpeed = guiData->spd*MPS_TO_KTS; //Speed in knots
  guiDepth = guiData->depth;
  guiRadarOn = guiData->radarOn;
  guiRadarRangeNm = guiData->radarRangeNm;
  guiTime = guiData->currentTime;
  guiPaused = guiData->paused;
  guiCollided = guiData->collided;
  // DEE Feb 23 vvvv height of tide
  guiTideHeight = guiData->tideHeight;


  radarHeadUp = guiData->headUp;

  //update EBL Data
  this->guiRadarEBLBrg = guiData->guiRadarEBLBrg;
  if (radarHeadUp) {
    this->guiRadarEBLBrg -= guiHeading;
  }
  this->guiRadarEBLRangeNm = guiData->guiRadarEBLRangeNm;

  //update cursor data
  this->guiRadarCursorBrg = guiData->guiRadarCursorBrg;
  if (radarHeadUp) {
    this->guiRadarCursorBrg -= guiHeading;
  }
  this->guiRadarCursorRangeNm = guiData->guiRadarCursorRangeNm;

  //Update ARPA data
  arpaContactStates = guiData->arpaContactStates;
  setARPAList(guiData->arpaListSelection);

}

void GUIMain::showLogWindow()
{

  irr::gui::IGUIWindow* logWindow = guienv->addWindow(irr::core::rect<irr::s32>(0.01*su,0.01*sh,0.99*su,0.99*sh));
  irr::gui::IGUIListBox* logText = guienv->addListBox(irr::core::rect<irr::s32>(0.03*su,0.05*sh,0.95*su,0.95*sh),logWindow);

  if (logWindow && logText && logMessages) {

    //logText->setDrawBackground(true);
    logText->clear();

    for (unsigned int i = 0; i<logMessages->size(); i++) {
      std::string logTextString = logMessages->at(i);
      logText->addItem(irr::core::stringw(logTextString.c_str()).c_str());
    }
  }

}

void GUIMain::drawGUI()
{
  //Remove big paused button when the simulation is started.
  if (pausedButton) {
    if (!guiPaused) {
      pausedButton->remove();
      pausedButton = 0;
    }
  }

  //Convert lat/long into a readable format
  wchar_t eastWest;
  wchar_t northSouth;
  if (guiLat >= 0) {
    northSouth='N';
  } else {
    northSouth='S';
  }
  if (guiLong >= 0) {
    eastWest='E';
  } else {
    eastWest='W';
  }
  irr::f32 displayLat = fabs(guiLat);
  irr::f32 displayLong = fabs(guiLong);

  irr::f32 latMinutes = (displayLat - (int)displayLat)*60;
  irr::f32 lonMinutes = (displayLong - (int)displayLong)*60;
  irr::u8 latDegrees = (int) displayLat;
  irr::u8 lonDegrees = (int) displayLong;

  //update heading display element
  irr::core::stringw displayText;

  displayText.append(language->translate("spd"));
  displayText.append(f32To1dp(guiSpeed).c_str());
  displayText.append(L" ");
  displayText.append(language->translate("kts"));
  displayText.append(L" ");

  if (showInterface) { //Only show speed in minimal 2d interface
    displayText.append(L"\n");
    if (hasDepthSounder) {
      displayText.append(language->translate("depth"));
      if (guiDepth <= maxSounderDepth) {
	displayText.append(f32To1dp(guiDepth).c_str());
      } else {
	displayText.append(L"-");
      }
      displayText.append(L" m \n");
    }

    displayText.append(irr::core::stringw(guiTime.c_str()));
    displayText.append(L"\n");

    if (hasGPS) {
      displayText.append(language->translate("pos"));
      displayText.append(irr::core::stringw(latDegrees));
      displayText.append(language->translate("deg"));
      displayText.append(f32To3dp(latMinutes).c_str());
      displayText.append(language->translate("minSymbol"));
      displayText.append(northSouth);
      displayText.append(L" ");

      displayText.append(irr::core::stringw(lonDegrees));
      displayText.append(language->translate("deg"));
      displayText.append(f32To3dp(lonMinutes).c_str());
      displayText.append(language->translate("minSymbol"));
      displayText.append(eastWest);
      displayText.append(L"\n");
    }

    displayText.append(language->translate("fps"));
    displayText.append(irr::core::stringw(device->getVideoDriver()->getFPS()).c_str());
    displayText.append(L"\n");

    if (showTideHeight) {
      // DEE FEB 23 vvv add height of tide to the display
      displayText.append(language->translate("hot"));
      displayText.append(f32To1dp(guiTideHeight).c_str());
      displayText.append(L"\n");
      // DEE FEB 23 ^^^
    }
	    
  }
  if (guiPaused) {
    displayText.append(language->translate("paused"));
    displayText.append(L"\n");
  }
  {
    // List box, not static text, so it gets a scrollbar automatically if the content overflows - split
    // the accumulated (newline-joined) text back out into one item per line.
    // This runs every frame, so preserve whatever scroll position the user has dragged to - otherwise
    // clear() resets it to the top each time and the box appears unscrollable.
    irr::s32 savedScrollPos = 0;
    if (dataDisplay->getVerticalScrollBar()) {
      savedScrollPos = dataDisplay->getVerticalScrollBar()->getPos();
    }
    dataDisplay->clear();
    irr::core::array<irr::core::stringw> displayLines;
    displayText.split(displayLines, L"\n", 1);
    for (irr::u32 i = 0; i < displayLines.size(); i++) {
      dataDisplay->addItem(displayLines[i].c_str());
    }
    if (dataDisplay->getVerticalScrollBar()) {
      dataDisplay->getVerticalScrollBar()->setPos(savedScrollPos);
    }
  }

  //add radar text (reuse the displayText)
  irr::f32 displayEBLBearing = guiRadarEBLBrg;
  irr::f32 displayCursorBearing = guiRadarCursorBrg;
  if (radarHeadUp) {
    displayEBLBearing += guiHeading;
    displayCursorBearing += guiHeading;
  }
  while (displayEBLBearing>=360) {displayEBLBearing-=360;}
  while (displayEBLBearing<0) {displayEBLBearing+=360;}
  while (displayCursorBearing>=360) {displayCursorBearing-=360;}
  while (displayCursorBearing<0) {displayCursorBearing+=360;}

  displayText = language->translate("range");
  displayText.append(f32To1dp(guiRadarRangeNm).c_str());
  displayText.append(language->translate("nm"));
  displayText.append(L"\n");

  displayText.append(language->translate("vrm"));
  displayText.append(f32To2dp(guiRadarEBLRangeNm).c_str());
  displayText.append(language->translate("nm"));
  if (guiRadarCursorRangeNm > 0){
    displayText.append(" ");
    displayText.append(language->translate("cursor"));
    displayText.append(f32To2dp(guiRadarCursorRangeNm).c_str());
    displayText.append(language->translate("nm"));
  }
  displayText.append(L"\n");

  displayText.append(language->translate("ebl"));
  displayText.append(f32To1dp(displayEBLBearing).c_str());
  displayText.append(language->translate("deg"));
  if (guiRadarCursorRangeNm > 0){
    displayText.append(" ");
    displayText.append(language->translate("cursor"));
    displayText.append(f32To2dp(displayCursorBearing).c_str());
    displayText.append(language->translate("deg"));
  }
  radarText ->setText(displayText.c_str());
  radarText2->setText(displayText.c_str());

  //Use guiCPAs and guiTCPAs to display ARPA data
  //Todo: Store current position and reset here
  irr::s32 selectedItem = arpaList->getSelected();
  irr::s32 selectedItem2 = arpaList2->getSelected();
  irr::s32 selectedPosition = 0;
  irr::s32 selectedPosition2 =0;
  if (arpaList->getVerticalScrollBar()) {selectedPosition=arpaList->getVerticalScrollBar()->getPos();}
  if (arpaList2->getVerticalScrollBar()) {selectedPosition2=arpaList2->getVerticalScrollBar()->getPos();}
  arpaList->clear();
  arpaList2->clear();
  arpaText->clear();
  arpaText2->clear();

  //if (guiCPAs.size() == guiTCPAs.size() && guiCPAs.size() == guiARPAspeeds.size() && guiCPAs.size() == guiARPAheadings.size()) {
  for (unsigned int i = 0; i < arpaContactStates.size(); i++) {

    //Convert TCPA from decimal minutes into minutes and seconds.
    //TODO: Filter list based on risk?

    // If stationary, show placeholder only
    if (arpaContactStates.at(i).stationary) {
      displayText = language->translate("untracked");
      arpaList->addItem(displayText.c_str());
      arpaList2->addItem(displayText.c_str());
      continue;
    }

    displayText = L"";

    irr::f32 tcpa = arpaContactStates.at(i).tcpa;
    irr::f32 cpa  = arpaContactStates.at(i).cpa;
    irr::u32 arpahdg = round(arpaContactStates.at(i).absHeading);
    irr::u32 arpaspd = round(arpaContactStates.at(i).speed);

    irr::u32 tcpaMins = floor(tcpa);
    irr::u32 tcpaSecs = floor(60*(tcpa - tcpaMins));

    irr::core::stringw tcpaDisplayMins = irr::core::stringw(tcpaMins);
    if (tcpaDisplayMins.size() == 1) {
      irr::core::stringw zeroPadded = L"0";
      zeroPadded.append(tcpaDisplayMins);
      tcpaDisplayMins = zeroPadded;
    }

    irr::core::stringw tcpaDisplaySecs = irr::core::stringw(tcpaSecs);
    if (tcpaDisplaySecs.size() == 1) {
      irr::core::stringw zeroPadded = L"0";
      zeroPadded.append(tcpaDisplaySecs);
      tcpaDisplaySecs = zeroPadded;
    }

    if (arpaContactStates.at(i).contactType == CONTACT_MANUAL) {
      displayText.append(language->translate("manualContact"));
    } else {
      displayText.append(language->translate("arpaContact"));
    }
    displayText.append(L" ");
    displayText.append(irr::core::stringw(i+1)); //Contact ID (1,2,...)
    displayText.append(L":");

    arpaList->addItem(displayText.c_str());
    arpaList2->addItem(displayText.c_str());

    if ( i==selectedItem || i==selectedItem2 ) {
      //Show arpa details

      if (arpaContactStates.at(i).range == 0) {
	// Exactly 0 means untracked
	displayText = language->translate("untracked");
	if (i==selectedItem) {
	  arpaText->addItem(displayText.c_str());
	}
	if (i==selectedItem2) {
	  arpaText2->addItem(displayText.c_str());
	}
      } else {
	//CPA
	displayText = L"";
	displayText.append(language->translate("cpa"));
	displayText.append(L":");
	displayText.append(f32To2dp(cpa).c_str());
	displayText.append(language->translate("nm"));
	//Add to the correct box
	if (i==selectedItem) {
	  arpaText->addItem(displayText.c_str());
	}
	if (i==selectedItem2) {
	  arpaText2->addItem(displayText.c_str());
	}

	//TCPA
	displayText = L"";
	displayText.append(language->translate("tcpa"));
	displayText.append(L":");
	if (tcpa >= 0) {
	  displayText.append(tcpaDisplayMins);
	  displayText.append(L":");
	  displayText.append(tcpaDisplaySecs);
	} else {
	  displayText.append(L" ");
	  displayText.append(language->translate("past"));
	}
	//Add to the correct box
	if (i==selectedItem) {
	  arpaText->addItem(displayText.c_str());
	}
	if (i==selectedItem2) {
	  arpaText2->addItem(displayText.c_str());
	}

	//Heading and speed
	//Pad heading to three decimals
	irr::core::stringw headingText = irr::core::stringw(arpahdg);
	if (headingText.size() == 1) {
	  irr::core::stringw zeroPadded = L"00";
	  zeroPadded.append(headingText);
	  headingText = zeroPadded;
	}
	else if (headingText.size() == 2) {
	  irr::core::stringw zeroPadded = L"0";
	  zeroPadded.append(headingText);
	  headingText = zeroPadded;
	}
	displayText = L"";
	displayText.append(headingText);
	displayText.append(language->translate("deg"));
	displayText.append(L" ");
	displayText.append(irr::core::stringw(arpaspd));
	displayText.append(L" kts");
	//Add to the correct box
	if (i==selectedItem) {
	  arpaText->addItem(displayText.c_str());
	}
	if (i==selectedItem2) {
	  arpaText2->addItem(displayText.c_str());
	}
      }

    }

  }
  //}
  if (selectedItem > -1 && (irr::s32)arpaList->getItemCount()>selectedItem) {
    arpaList->setSelected(selectedItem);
  }
  if (selectedItem2 > -1 && (irr::s32)arpaList2->getItemCount()>selectedItem2) {
    arpaList2->setSelected(selectedItem2);
  }
  if(arpaList->getVerticalScrollBar()) {
    arpaList->getVerticalScrollBar()->setPos(selectedPosition);
  }
  if(arpaList2->getVerticalScrollBar()) {
    arpaList2->getVerticalScrollBar()->setPos(selectedPosition2);
  }


  //add a collision warning
  if (guiCollided && showCollided) {
    drawCollisionWarning();
  }

  //manually trigger gui event if buttons are held down
  if (eblUpButton->isPressed()) {manuallyTriggerClick(eblUpButton);}
  if (eblDownButton->isPressed()) {manuallyTriggerClick(eblDownButton);}
  if (eblLeftButton->isPressed()) {manuallyTriggerClick(eblLeftButton);}
  if (eblRightButton->isPressed()) {manuallyTriggerClick(eblRightButton);}

  if (eblUpButton2->isPressed()) {manuallyTriggerClick(eblUpButton2);}
  if (eblDownButton2->isPressed()) {manuallyTriggerClick(eblDownButton2);}
  if (eblLeftButton2->isPressed()) {manuallyTriggerClick(eblLeftButton2);}
  if (eblRightButton2->isPressed()) {manuallyTriggerClick(eblRightButton2);}

  if (bowThrusterLeftButton && bowThrusterLeftButton->isPressed()) {manuallyTriggerClick(bowThrusterLeftButton);}
  if (bowThrusterRightButton && bowThrusterRightButton->isPressed()) {manuallyTriggerClick(bowThrusterRightButton);}
  if (sternThrusterLeftButton && sternThrusterLeftButton->isPressed()) {manuallyTriggerClick(sternThrusterLeftButton);}
  if (sternThrusterRightButton && sternThrusterRightButton->isPressed()) {manuallyTriggerClick(sternThrusterRightButton);}

  if (radarCursorLeftButton->isPressed()) {manuallyTriggerClick(radarCursorLeftButton);}
  if (radarCursorRightButton->isPressed()) {manuallyTriggerClick(radarCursorRightButton);}
  if (radarCursorUpButton->isPressed()) {manuallyTriggerClick(radarCursorUpButton);}
  if (radarCursorDownButton->isPressed()) {manuallyTriggerClick(radarCursorDownButton);}

  if (radarCursorLeftButton2->isPressed()) {manuallyTriggerClick(radarCursorLeftButton2);}
  if (radarCursorRightButton2->isPressed()) {manuallyTriggerClick(radarCursorRightButton2);}
  if (radarCursorUpButton2->isPressed()) {manuallyTriggerClick(radarCursorUpButton2);}
  if (radarCursorDownButton2->isPressed()) {manuallyTriggerClick(radarCursorDownButton2);}

  if (nonFollowUpPortButton) {
    //Step the wheel towards port while held, same rate-limited click mechanism as the thruster buttons
    nfuPortDown = nonFollowUpPortButton->isPressed();
    if (nfuPortDown) {manuallyTriggerClick(nonFollowUpPortButton);}
  }

  if (nonFollowUpStbdButton) {
    //Step the wheel towards stbd while held, same rate-limited click mechanism as the thruster buttons
    nfuStbdDown = nonFollowUpStbdButton->isPressed();
    if (nfuStbdDown) {manuallyTriggerClick(nonFollowUpStbdButton);}
  }

  // Update lines display
  if (mOwnShip && mLines) {
    std::vector<std::string> linesNames = mLines->getLineNames();

    // remove excess lines if required
    while (linesList->getItemCount() > linesNames.size()) {
      linesList->removeItem(linesList->getItemCount() - 1);
      linesList->setSelected(-1);
    }

    // Update text for existing lines
    for (unsigned int i = 0; i < linesList->getItemCount(); i++) {
      linesList->setItem(i, irr::core::stringw(linesNames.at(i).c_str()).c_str(), -1);
    }
            
    // Add additional lines if required
    for (unsigned int i = linesList->getItemCount(); i < linesNames.size(); i++) {
      linesList->addItem(irr::core::stringw(linesNames.at(i).c_str()).c_str());
    }

    // Get 'keepSlack' & 'heaveIn' status of current line
    if (linesList->getSelected() > -1) {
      keepLineSlack->setChecked(mLines->getKeepSlack(linesList->getSelected()));
      heaveLineIn->setChecked(mLines->getHeaveIn(linesList->getSelected()));
    } else {
      keepLineSlack->setChecked(false);
      heaveLineIn->setChecked(false);
    }


  }

  guienv->drawAll();

  //draw the heading line on the radar
  if (showInterface || radarLarge) {
    if (guiRadarOn) {
      draw2dRadar();
    }
  }

  //draw view bearing if needed
  if (bearingButton->isPressed()){
    draw2dBearing();
  }
}

void GUIMain::draw2dRadar()
{
  irr::s32 centreX;
  irr::s32 centreY;
  irr::s32 radius;

  if (radarLarge) {
    centreX = largeRadarScreenCentreX;
    centreY = largeRadarScreenCentreY;
    radius = largeRadarScreenRadius;
  } else {
    centreX = smallRadarScreenCentreX;
    centreY = smallRadarScreenCentreY;
    radius = smallRadarScreenRadius;
  }

  //std::cout << radius*2 << std::endl;

  //If full screen radar, draw a 4:3 box around the radar display area
  if (radarLarge) {
    device->getVideoDriver()->draw2DRectangleOutline(radarLargeRect,irr::video::SColor(255,0,0,0));
  }

  irr::f32 radarHeadingIndicator;
  if (radarHeadUp) {
    radarHeadingIndicator = 0;
  } else {
    radarHeadingIndicator = guiHeading;
  }
  irr::s32 deltaX = radius*sin(irr::core::DEGTORAD*radarHeadingIndicator);
  irr::s32 deltaY = -1*radius*cos(irr::core::DEGTORAD*radarHeadingIndicator);
  irr::core::position2d<irr::s32> radarCentre (centreX,centreY);
  irr::core::position2d<irr::s32> radarHeading (centreX+deltaX,centreY+deltaY);
  device->getVideoDriver()->draw2DLine(radarCentre,radarHeading,irr::video::SColor(255, 255, 255, 255)); //Todo: Make these colours configurable

  //draw a look direction line
  if (radarHeadUp) {
    radarHeadingIndicator = viewHdg - guiHeading;
  } else {
    radarHeadingIndicator = viewHdg;
  }
  irr::s32 deltaXView = radius*sin(irr::core::DEGTORAD*radarHeadingIndicator);
  irr::s32 deltaYView = -1*radius*cos(irr::core::DEGTORAD*radarHeadingIndicator);
  irr::core::position2d<irr::s32> lookInner (centreX + 0.9*deltaXView,centreY + 0.9*deltaYView);
  irr::core::position2d<irr::s32> lookOuter (centreX + deltaXView,centreY + deltaYView);
  device->getVideoDriver()->draw2DLine(lookInner,lookOuter,irr::video::SColor(255, 255, 0, 0)); //Todo: Make these colours configurable

  //draw an EBL line
  irr::s32 deltaXEBL = radius*sin(irr::core::DEGTORAD*guiRadarEBLBrg);
  irr::s32 deltaYEBL = -1*radius*cos(irr::core::DEGTORAD*guiRadarEBLBrg);
  irr::core::position2d<irr::s32> eblOuter (centreX + deltaXEBL,centreY + deltaYEBL);
  device->getVideoDriver()->draw2DLine(radarCentre,eblOuter,irr::video::SColor(255, 255, 0, 0));
  //draw EBL range
  if (guiRadarEBLRangeNm > 0 && guiRadarRangeNm >= guiRadarEBLRangeNm) {
    irr::f32 eblRangePx = radius*guiRadarEBLRangeNm/guiRadarRangeNm;
    irr::u8 noSegments = eblRangePx/2;
    if (noSegments < 10) {noSegments=10;}
    device->getVideoDriver()->draw2DPolygon(radarCentre,eblRangePx,irr::video::SColor(255, 255, 0, 0),noSegments); //An n segment polygon, to approximate a circle
  }

  //draw radar cursor
  irr::s32 cursorPixelRadius = radius*guiRadarCursorRangeNm/guiRadarRangeNm;
  irr::s32 deltaXCursor = cursorPixelRadius*sin(irr::core::DEGTORAD*guiRadarCursorBrg);
  irr::s32 deltaYCursor = -1*cursorPixelRadius*cos(irr::core::DEGTORAD*guiRadarCursorBrg);
  //Plot if within the display and not at zero range
  if (cursorPixelRadius <= radius && guiRadarCursorRangeNm > 0) {
    irr::core::position2d<irr::s32> cursorCentre (centreX + deltaXCursor,centreY + deltaYCursor);
    device->getVideoDriver()->draw2DPolygon(cursorCentre,radius/20,irr::video::SColor(255, 255, 0, 0),4); //a 4 segment polygon, i.e. a square!
  }

  //Draw compass rose around radar (?Rotate with radar in head up and course up?)
  for (irr::u32 ticAngle = 0; ticAngle < 360; ticAngle += 5) {

    irr::f32 displayTicAngle = ticAngle;
    if (radarHeadUp) {
      displayTicAngle -= guiHeading;
    }

    irr::f32 scaling = 0.98;
    bool showValue = false;
    if(ticAngle % 20 == 0 ) {
      scaling = 0.90;
      showValue = true;
    } else if (ticAngle % 10 == 0) {
      scaling = 0.94;
    }

    irr::s32 deltaXTic = radius*sin(irr::core::DEGTORAD*displayTicAngle);
    irr::s32 deltaYTic = -1*radius*cos(irr::core::DEGTORAD*displayTicAngle);
    irr::core::position2d<irr::s32> ticInner (centreX + scaling*deltaXTic,centreY + scaling*deltaYTic);
    irr::core::position2d<irr::s32> ticOuter (centreX + deltaXTic,centreY + deltaYTic);

    device->getVideoDriver()->draw2DLine(ticInner,ticOuter,irr::video::SColor(255, 128, 128, 128));

    //Show the angle if needed
    if (showValue) {

      irr::core::stringw angleText = irr::core::stringw(ticAngle);

      irr::s32 textWidth = guienv->getSkin()->getFont()->getDimension(angleText.c_str()).Width;
      irr::s32 textHeight = guienv->getSkin()->getFont()->getDimension(angleText.c_str()).Height;
      irr::s32 textStartX = centreX + 0.8*deltaXTic-0.5*textWidth;
      irr::s32 textEndX = textStartX+textWidth;
      irr::s32 textStartY = centreY + 0.8*deltaYTic-0.5*textHeight;
      irr::s32 textEndY = textStartY+textHeight;
      guienv->getSkin()->getFont()->draw(angleText,irr::core::rect<irr::s32>(textStartX,textStartY,textEndX,textEndY),irr::video::SColor(255,128,128,128));
    }

  }

  //Draw range rings

  //Draw 4 range rings if radar range is divisible by 1.5, otherwise draw 4
  irr::u32 rangeRings;
  if ( std::fmod(guiRadarRangeNm,1.5) < 0.1) {
    rangeRings = 3;
  } else {
    rangeRings = 4;
  }
  for (unsigned int i = 1; i<rangeRings; i++) {
    irr::f32 ringRadius = radius*i/(float)rangeRings;
    irr::u8 noSegments = ringRadius/2;
    device->getVideoDriver()->draw2DPolygon(radarCentre,ringRadius,irr::video::SColor(128, 128, 128, 128),noSegments);
  }

}

void GUIMain::draw2dBearing()
{

  //make cross hairs
  irr::s32 screenCentreX = 0.5*su;
  irr::s32 screenCentreY;
  if (showInterface) {
    screenCentreY = 0.3*sh;
  } else {
    screenCentreY = 0.5*sh;
  }
  irr::s32 lineLength = 0.1*sh;
  irr::core::position2d<irr::s32> left(screenCentreX-lineLength,screenCentreY);
  irr::core::position2d<irr::s32> right(screenCentreX+lineLength,screenCentreY);
  irr::core::position2d<irr::s32> top(screenCentreX,screenCentreY-lineLength);
  irr::core::position2d<irr::s32> bottom(screenCentreX,screenCentreY+lineLength);
  irr::core::position2d<irr::s32> centre(screenCentreX,screenCentreY);
  device->getVideoDriver()->draw2DLine(left,right,irr::video::SColor(255, 255, 0, 0));
  device->getVideoDriver()->draw2DLine(top,bottom,irr::video::SColor(255, 255, 0, 0));

  //show view bearing
  guienv->getSkin()->getFont()->draw(f32To1dp(viewHdg).c_str(),irr::core::rect<irr::s32>(screenCentreX-lineLength,screenCentreY-lineLength,screenCentreX, screenCentreY), irr::video::SColor(255,255,0,0),true,true);
  guienv->getSkin()->getFont()->draw(f32To1dp(viewElev).c_str(),irr::core::rect<irr::s32>(screenCentreX-lineLength,screenCentreY,screenCentreX, screenCentreY+lineLength), irr::video::SColor(255,255,0,0),true,true);


  //show angle (from horizon)

}

void GUIMain::drawCollisionWarning()
{
  irr::s32 screenCentreX = 0.5*su;
  irr::s32 screenCentreY = 0.05*sh;

  device->getVideoDriver()->draw2DRectangle(irr::video::SColor(255,255,255,255),irr::core::rect<irr::s32>(screenCentreX-0.25*su,screenCentreY-0.025*sh,screenCentreX+0.25*su, screenCentreY+0.025*sh));
  guienv->getSkin()->getFont()->draw(language->translate("collided"),
				     irr::core::rect<irr::s32>(screenCentreX-0.25*su,screenCentreY-0.025*sh,screenCentreX+0.25*su, screenCentreY+0.025*sh),
				     irr::video::SColor(255,255,0,0),true,true);
}

void GUIMain::setExtraControlsWindowVisible(bool windowVisible)
{
  extraControlsWindow->setVisible(windowVisible);
  if (windowVisible) {
    guienv->setFocus(extraControlsWindow);
  }
}

void GUIMain::setLinesControlsWindowVisible(bool windowVisible)
{
  linesControlsWindow->setVisible(windowVisible);
  if (windowVisible) {
    guienv->setFocus(linesControlsWindow);
  }
}

void GUIMain::setLinesControlsText(std::string textToShow)
{
  linesText->setText(irr::core::stringw(textToShow.c_str()).c_str());
}
