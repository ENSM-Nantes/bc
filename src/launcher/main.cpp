//Common launcher program
//This just launches Bridge Command or
//Map Controller executable depending
//on which button the user presses

/*
 * Icons from :
 * https://github.com/dubdubdubco/iconicicons.git
 * CC0 Public domain 
 */

#ifdef _MSC_VER
#pragma comment(linker, "/subsystem:windows /ENTRY:mainCRTStartup")
#endif

#include "irrlicht.h"
#include <iostream>
#include <thread>
#include <vector>
#include "../IniFile.hpp"
#include "../Lang.hpp"
#include "../Utilities.hpp"
#include "../Constants.hpp"
#include "../Credits.hpp"

//headers for execl
#ifdef _WIN32
#include <windows.h>
#include <process.h>
#include <direct.h>
#else
#include <unistd.h>
#endif

// Irrlicht Namespaces
//using namespace irr;

const int FONT_SIZE_DEFAULT = 12;

//Global definition for ini logger
namespace IniFile {
  irr::ILogger* irrlichtLogger = 0;
}

const irr::s32 BC_BUTTON = 1;
const irr::s32 MC_BUTTON = 2;
const irr::s32 RP_BUTTON = 3;
const irr::s32 ED_BUTTON = 4;
const irr::s32 MH_BUTTON = 5;
const irr::s32 INI_BC_BUTTON = 6;
const irr::s32 INI_MC_BUTTON = 7;
const irr::s32 INI_RP_BUTTON = 8;
const irr::s32 INI_MH_BUTTON = 9;
const irr::s32 SHIP_ED_BUTTON = 10;
const irr::s32 DOC_BUTTON = 11;
const irr::s32 USER_BUTTON = 12;
const irr::s32 EXIT_BUTTON = 13;
const irr::s32 CREDITS_BUTTON = 14;

const irr::s32 CREDITS_WINDOW_ID = 100;
const irr::s32 CREDITS_CLOSE_BUTTON = 101;
const irr::s32 CREDITS_SCROLLBAR_ID = 102;

std::string userFolder;

//Event receiver used only while the credits popup is open: handles closing it
//(Close button, Escape) and scrolling the credits text (scrollbar, mouse wheel)
class CreditsReceiver : public irr::IEventReceiver
{
public:
  CreditsReceiver(irr::gui::IGUIStaticText* aText, irr::gui::IGUIScrollBar* aScrollBar, irr::s32 aTextTop)
    : text(aText), scrollBar(aScrollBar), textTop(aTextTop), closed(false) { }

  virtual bool OnEvent(const irr::SEvent& event)
  {
    if (event.EventType == irr::EET_GUI_EVENT) {
      if (event.GUIEvent.EventType == irr::gui::EGET_BUTTON_CLICKED
          && event.GUIEvent.Caller->getID() == CREDITS_CLOSE_BUTTON) {
	closed = true;
      }
      if (event.GUIEvent.EventType == irr::gui::EGET_SCROLL_BAR_CHANGED
          && event.GUIEvent.Caller->getID() == CREDITS_SCROLLBAR_ID) {
	applyScroll();
      }
    }
    if (event.EventType == irr::EET_KEY_INPUT_EVENT
        && event.KeyInput.PressedDown && event.KeyInput.Key == irr::KEY_ESCAPE) {
      closed = true;
    }
    if (event.EventType == irr::EET_MOUSE_INPUT_EVENT
        && event.MouseInput.Event == irr::EMIE_MOUSE_WHEEL) {
      irr::s32 newPos = scrollBar->getPos() - (irr::s32)(event.MouseInput.Wheel * 40);
      if (newPos < scrollBar->getMin()) {newPos = scrollBar->getMin();}
      if (newPos > scrollBar->getMax()) {newPos = scrollBar->getMax();}
      scrollBar->setPos(newPos);
      applyScroll();
    }
    return false;
  }

  bool isClosed() const { return closed; }

private:
  void applyScroll()
  {
    irr::core::rect<irr::s32> r = text->getRelativePosition();
    irr::s32 height = r.getHeight();
    r.UpperLeftCorner.Y = textTop - scrollBar->getPos();
    r.LowerRightCorner.Y = r.UpperLeftCorner.Y + height;
    text->setRelativePosition(r);
  }

  irr::gui::IGUIStaticText* text;
  irr::gui::IGUIScrollBar* scrollBar;
  irr::s32 textTop;
  bool closed;
};

//Shows the credits in a scrollable modal popup over the launcher window
void showCreditsPopup(irr::IrrlichtDevice* device, Lang& language)
{
  irr::video::IVideoDriver* driver = device->getVideoDriver();
  irr::gui::IGUIEnvironment* gui = device->getGUIEnvironment();

  irr::core::dimension2d<irr::u32> screenSize = driver->getScreenSize();
  irr::s32 su = (irr::s32)screenSize.Width;
  irr::s32 sh = (irr::s32)screenSize.Height;

  //Hide the launcher's own buttons while the popup is open - the default skin draws
  //window/button faces with partial transparency, so anything left visible behind the
  //popup bleeds through it
  irr::gui::IGUIElement* root = gui->getRootGUIElement();
  std::vector<irr::gui::IGUIElement*> hiddenElements;
  for (irr::gui::IGUIElement* child : root->getChildren()) {
    hiddenElements.push_back(child);
    child->setVisible(false);
  }

  irr::s32 margin = su / 12;
  if (margin < 6) {margin = 6;}
  irr::s32 closeButtonHeight = 26;
  irr::s32 closeButtonGap = 8;

  //The window stops short of the bottom margin, leaving room for the Close button to sit
  //below it - kept outside the window so it can't be overdrawn by the tall (clipped) text
  irr::core::rect<irr::s32> windowRect(margin, margin, su - margin, sh - margin - closeButtonHeight - closeButtonGap);
  //Not modal: we already hide every other element above, and a modal window restricts input
  //to its own subtree, which would swallow clicks on the Close button living outside it
  irr::gui::IGUIWindow* creditsWindow = gui->addWindow(windowRect, false, language.translate("creditsTitle").c_str(), 0, CREDITS_WINDOW_ID);
  //We provide our own Close button below, so hide the window's built-in one to keep a single, unambiguous close path
  creditsWindow->getCloseButton()->setVisible(false);
  creditsWindow->setDraggable(false);

  irr::core::rect<irr::s32> clientRect = creditsWindow->getClientRect();
  irr::s32 scrollBarWidth = 16;
  irr::s32 pad = 6;

  irr::core::rect<irr::s32> visibleTextRect(
    clientRect.UpperLeftCorner.X + pad,
    clientRect.UpperLeftCorner.Y + pad,
    clientRect.LowerRightCorner.X - scrollBarWidth - pad,
    clientRect.LowerRightCorner.Y - pad
  );

  //Tall rect so the whole word-wrapped text exists; the window clips it to visibleTextRect's
  //height for us, and we scroll by shifting this rect's Y - a standard Irrlicht scrolling trick
  irr::core::rect<irr::s32> fullTextRect(
    visibleTextRect.UpperLeftCorner.X,
    visibleTextRect.UpperLeftCorner.Y,
    visibleTextRect.LowerRightCorner.X,
    visibleTextRect.UpperLeftCorner.Y + 6000
  );

  irr::gui::IGUIStaticText* creditsText = gui->addStaticText(getCredits().c_str(), fullTextRect, false, true, creditsWindow);

  irr::s32 visibleHeight = visibleTextRect.getHeight();
  irr::s32 maxScroll = creditsText->getTextHeight() - visibleHeight;
  if (maxScroll < 0) {maxScroll = 0;}

  irr::core::rect<irr::s32> scrollRect(
    clientRect.LowerRightCorner.X - scrollBarWidth,
    visibleTextRect.UpperLeftCorner.Y,
    clientRect.LowerRightCorner.X,
    visibleTextRect.LowerRightCorner.Y
  );
  irr::gui::IGUIScrollBar* scrollBar = gui->addScrollBar(false, scrollRect, creditsWindow, CREDITS_SCROLLBAR_ID);
  scrollBar->setMin(0);
  scrollBar->setMax(maxScroll);
  scrollBar->setSmallStep(20);
  irr::s32 largeStep = visibleHeight - 20;
  if (largeStep < 20) {largeStep = 20;}
  scrollBar->setLargeStep(largeStep);
  scrollBar->setPos(0);

  irr::s32 centreX = su / 2;
  irr::core::rect<irr::s32> closeButtonRect(
    centreX - 40, sh - margin - closeButtonHeight,
    centreX + 40, sh - margin
  );
  irr::gui::IGUIButton* closeButton = gui->addButton(closeButtonRect, 0, CREDITS_CLOSE_BUTTON, language.translate("creditsClose").c_str());

  CreditsReceiver creditsReceiver(creditsText, scrollBar, visibleTextRect.UpperLeftCorner.Y);
  irr::IEventReceiver* oldReceiver = device->getEventReceiver();
  device->setEventReceiver(&creditsReceiver);

  //Flush old key/clicks etc, so the click that opened this popup doesn't also close it
  device->sleep(200);
  device->clearSystemMessages();

  while (device->run() && !creditsReceiver.isClosed()) {
    driver->beginScene(irr::video::ECBF_COLOR | irr::video::ECBF_DEPTH, irr::video::SColor(0, 200, 200, 200));
    gui->drawAll();
    driver->endScene();
    device->sleep(10);
  }

  device->setEventReceiver(oldReceiver);
  creditsWindow->remove();
  closeButton->remove();

  for (irr::gui::IGUIElement* el : hiddenElements) {
    el->setVisible(true);
  }
}

//Event receiver: This does the actual launching
class Receiver : public irr::IEventReceiver
{
public:
  Receiver(irr::IrrlichtDevice* aDevice, Lang* aLanguage) : device(aDevice), language(aLanguage) { }

  virtual bool OnEvent(const irr::SEvent& event)
  {
    if (event.EventType == irr::EET_GUI_EVENT) {
      if (event.GUIEvent.EventType == irr::gui::EGET_BUTTON_CLICKED ) {
	irr::s32 id = event.GUIEvent.Caller->getID();

	if (id == EXIT_BUTTON) {
	  exit(EXIT_SUCCESS);
	}

	if (id == CREDITS_BUTTON) {
	  showCreditsPopup(device, *language);
	}

	if (id == BC_BUTTON) {
#ifdef _WIN32
	  ShellExecute(NULL, NULL, "bridgecommand-bc.exe", NULL, NULL, SW_SHOW);
	  //_execl("./bridgecommand-bc.exe", "bridgecommand-bc.exe", NULL);
#else
	  execl("./bridgecommand-bc", "bridgecommand-bc", NULL);
#endif

	}
	if (id == MC_BUTTON) {
#ifdef _WIN32
	  ShellExecute(NULL, NULL, "bridgecommand-mc.exe", NULL, NULL, SW_SHOW);
	  //_execl("./bridgecommand-mc.exe", "bridgecommand-mc.exe", NULL);
#else
	  //Other (assumed posix)
	  execl("./bridgecommand-mc", "bridgecommand-mc", NULL);
#endif
	}
	if (id == RP_BUTTON) {
#ifdef _WIN32
	  ShellExecute(NULL, NULL, "bridgecommand-rp.exe", NULL, NULL, SW_SHOW);
	  //_execl("./bridgecommand-rp.exe", "bridgecommand-rp.exe", NULL);
#else
	  //Other (assumed posix)
	  execl("./bridgecommand-rp", "bridgecommand-rp", NULL);
#endif
	}
	if (id == ED_BUTTON) {
#ifdef _WIN32
	  ShellExecute(NULL, NULL, "bridgecommand-ed.exe", NULL, NULL, SW_SHOW);
	  //_execl("./bridgecommand-ed.exe", "bridgecommand-ed.exe", NULL);
#else
	  //Other (assumed posix)
	  execl("./bridgecommand-ed", "bridgecommand-ed", NULL);
#endif
	}
	if (id == MH_BUTTON) {
#ifdef _WIN32
	  ShellExecute(NULL, NULL, "bridgecommand-mh.exe", NULL, NULL, SW_SHOW);
	  //_execl("./bridgecommand-mh.exe", "bridgecommand-mh.exe", NULL);
#else
	  //Other (assumed posix)
	  execl("./bridgecommand-mh", "bridgecommand-mh", NULL);
#endif
	}
	if (id == INI_BC_BUTTON) {
#ifdef _WIN32
	  ShellExecute(NULL, NULL, "bridgecommand-ini.exe", NULL, NULL, SW_SHOW);
	  //_execl("./bridgecommand-ini.exe", "bridgecommand-ini.exe", NULL);
#else
	  //Other (assumed posix)
	  execl("./bridgecommand-ini", "bridgecommand-ini", NULL);
#endif
	}
	if (id == INI_MC_BUTTON) {
#ifdef _WIN32
	  ShellExecute(NULL, NULL, "bridgecommand-ini.exe", "-M", NULL, SW_SHOW);
	  //_execl("./bridgecommand-ini.exe", "bridgecommand-ini.exe", "-M", NULL);
#else
	  //Other (assumed posix)
	  execl("./bridgecommand-ini", "bridgecommand-ini", "-M", NULL);
#endif
	}
	if (id == INI_RP_BUTTON) {
#ifdef _WIN32
	  ShellExecute(NULL, NULL, "bridgecommand-ini.exe", "-R", NULL, SW_SHOW);
	  //_execl("./bridgecommand-ini.exe", "bridgecommand-ini.exe", "-R", NULL);
#else
	  //Other (assumed posix)
	  execl("./bridgecommand-ini", "bridgecommand-ini", "-R", NULL);
#endif
	}
	if (id == INI_MH_BUTTON) {
#ifdef _WIN32
	  ShellExecute(NULL, NULL, "bridgecommand-ini.exe", "-H", NULL, SW_SHOW);
	  //_execl("./bridgecommand-ini.exe", "bridgecommand-ini.exe", "-H", NULL);
#else
	  //Other (assumed posix)
	  execl("./bridgecommand-ini", "bridgecommand-ini", "-H", NULL);
#endif
	}
	if (id == DOC_BUTTON) {
#ifdef _WIN32
	  //CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	  ShellExecute(NULL, TEXT("open"), TEXT("doc\\index.html"), NULL, NULL, SW_SHOWNORMAL);
	  //Sleep(5000);
	  //exit(EXIT_SUCCESS);
#else
	  execl("/usr/bin/xdg-open", "xdg-open", "doc/index.html", NULL);
	  //If execuation gets to this point, it has failed to launch help. Try to fall back to online documentation	 
	  execl("/usr/bin/xdg-open", "xdg-open", "https://www.bridgecommand.co.uk/Documentation", NULL);
#endif
	}
	if (id == USER_BUTTON) {
#ifdef _WIN32
	  //CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	  ShellExecute(NULL, TEXT("open"), TEXT(userFolder.c_str()), NULL, NULL, SW_SHOWNORMAL);
	  //Sleep(5000);
	  //exit(EXIT_SUCCESS);
#else
	  //Other (assumed posix)
	  execl("/usr/bin/xdg-open", "xdg-open", userFolder.c_str(), NULL);
#endif
	}
	if (id == SHIP_ED_BUTTON) {
#ifdef _WIN32
		char oldDir[1024];
		GetCurrentDirectory(1024, oldDir);
		SetCurrentDirectory("C:\\Program Files\\ShipEditor\\res");
		ShellExecute(NULL, NULL, "C:\\Program Files\\ShipEditor\\build\\ShipEditor.exe", NULL, NULL, SW_SHOW);
		SetCurrentDirectory(oldDir);
#else
		execl("./ShipEditor", "ShipEditor", NULL);
		execl("../../../ShipEditor/res/launch.sh", "ShipEditor", NULL);
#endif
	}
      }
    }
    if (event.EventType == irr::EET_KEY_INPUT_EVENT) {
      if (event.KeyInput.Key == irr::KEY_ESCAPE ) {
	exit(EXIT_SUCCESS);
      }
    }
    return false;
  }

private:
  irr::IrrlichtDevice* device;
  Lang* language;
};

int main (int argc, char ** argv)
{

  if ((argc>1)&&(strcmp(argv[1],"--version")==0)) {
    std::cout << VERSION   << std::endl;
    exit(EXIT_SUCCESS);
  }

  char cwd[1024]={0};

  if(0 != CHDIR("../../resources/"))//Launch from builded sources
    {
      if(0 != CHDIR("/usr/share/bridgecommand"))//Launch from install
	{
	  std::cout << "Bidge Commands not able to get resources files" << std::endl;
	  exit(-1);
	}
    }

  if(GETCWD(cwd, sizeof(cwd)) != NULL) printf("Launcher::Working Directory : %s\n", cwd);
    
  //User read/write location - look in here first and the exe folder second for files
  userFolder = Utilities::getUserDir();

  //Read basic ini settings
  std::string iniFilename = "bc5.ini";

  if(Utilities::pathExists(userFolder + "bc5.ini"))
    {
      iniFilename = userFolder + "bc5.ini";
    }
  
  std::string modifier = IniFile::iniFileToString(iniFilename, "lang");
  if (modifier.length()==0) {
    modifier = "en"; //Default
  }
  std::string languageFile = "lang/languageLauncher-";

  languageFile.append(modifier);
  languageFile.append(".txt");
  if (Utilities::pathExists(userFolder + languageFile)) {
    languageFile = userFolder + languageFile;
  }

  Lang language(languageFile);

  int fontSize = FONT_SIZE_DEFAULT;
  float fontScale = IniFile::iniFileTof32(iniFilename, "font_scale");
  if (fontScale > 1) {
    fontSize = (int)(fontSize * fontScale + 0.5);
  } else {
    fontScale = 1.0;
  }

  irr::u32 graphicsWidth = 300;
  irr::u32 graphicsHeight = 680;
  irr::u32 graphicsDepth = 32;
  bool fullScreen = false;

  irr::SIrrlichtCreationParameters deviceParameters;
  deviceParameters.DriverType = irr::video::EDT_OPENGL;
  deviceParameters.WindowSize = irr::core::dimension2d<irr::u32>(graphicsWidth, graphicsHeight);
  deviceParameters.Bits = graphicsDepth;
  deviceParameters.Fullscreen = fullScreen;

  irr::IrrlichtDevice* device = irr::createDeviceEx(deviceParameters);
  irr::video::IVideoDriver* driver = device->getVideoDriver();
  
  irr::video::ITexture* imgTexture = driver->getTexture("media/logo.png");
  irr::core::dimension2d<irr::u32> imgSize = imgTexture->getSize();

  driver->OnResize(irr::core::dimension2d<irr::u32>(imgSize.Width, graphicsHeight));

#ifdef __APPLE__
  //Mac OS - cd back to original dir - seems to be changed during createDevice
  irr::io::IFileSystem* fileSystem = device->getFileSystem();
  if (fileSystem==0) {
    exit(EXIT_FAILURE); //Could not get file system TODO: Message for user
    std::cout << "Could not get filesystem" << std::endl;
  }
  fileSystem->changeWorkingDirectoryTo(exeFolderPath.c_str());
#endif

  device->setWindowCaption(L"Bridge Command");
  irr::gui::IGUISkin* newskin = device->getGUIEnvironment()->createSkin(irr::gui::EGST_WINDOWS_CLASSIC);
  device->getGUIEnvironment()->setSkin(newskin);

  std::string fontName = IniFile::iniFileToString(iniFilename, "font");
  std::string fontPath = "media/fonts/" + fontName + "/" + fontName + "-" + std::to_string(fontSize) + ".xml";
  irr::gui::IGUIFont *font = device->getGUIEnvironment()->getFont(fontPath.c_str());


  if (font == NULL) {
    std::cout << "Could not load font " << fontPath << std::endl;
  } else {
    //set skin default font
    device->getGUIEnvironment()->getSkin()->setFont(font);
    device->getGUIEnvironment()->getSkin()->setFont(device->getGUIEnvironment()->getBuiltInFont(), irr::gui::EGDF_TOOLTIP);
  }

    
  //Add launcher buttons with layout in viewport due to scaling

  short bC = graphicsWidth / 20;       // padding ...
  short bR = bC / 3 * 2 + 1;

  short bW = graphicsWidth - (2 * bC); // size ...
  short bH = 20 * (fontSize / (FONT_SIZE_DEFAULT * 1.0)) + 1;

  short x1 = bC;                       // location ...
  short x2 = x1 + bW;
  short y1, y2;

  device->getGUIEnvironment()->addImage(imgTexture, irr::core::position2d<int>(bC, 10));

  y1 = imgSize.Height +   2*bR; y2 = y1 + 2*bH; 
  irr::gui::IGUIButton* launchBC    = device->getGUIEnvironment()->addButton(irr::core::rect<irr::s32>(x1,y1,x2,y2),0,BC_BUTTON,language.translate("startBC").c_str()); //i18n
  launchBC->setImage(driver->getTexture("media/startBC.png"));
  launchBC->setUseAlphaChannel();

  y1 = y2 + 3*bR; y2 = y1 +   bH; irr::gui::IGUIButton* launchED    = device->getGUIEnvironment()->addButton(irr::core::rect<irr::s32>(x1,y1,x2,y2),0,ED_BUTTON,language.translate("startED").c_str()); //i18n
  launchED->setImage(driver->getTexture("media/startED.png"));
  launchED->setUseAlphaChannel();

  y1 = y2 + bR; y2 = y1 +   bH; irr::gui::IGUIButton* launchSE    = device->getGUIEnvironment()->addButton(irr::core::rect<irr::s32>(x1,y1,x2,y2),0,SHIP_ED_BUTTON,language.translate("startSE").c_str()); //i18n
  launchSE->setImage(driver->getTexture("media/startSE.png"));
  launchSE->setUseAlphaChannel();

  
  y1 = y2 +   bR; y2 = y1 +   bH; irr::gui::IGUIButton* launchMC    = device->getGUIEnvironment()->addButton(irr::core::rect<irr::s32>(x1,y1,x2,y2),0,MC_BUTTON,language.translate("startMC").c_str()); //i18n
  launchMC->setImage(driver->getTexture("media/startMC.png"));
  launchMC->setUseAlphaChannel();
  y1 = y2 +   bR; y2 = y1 +   bH; irr::gui::IGUIButton* launchRP    = device->getGUIEnvironment()->addButton(irr::core::rect<irr::s32>(x1,y1,x2,y2),0,RP_BUTTON,language.translate("startRP").c_str()); //i18n
  launchRP->setImage(driver->getTexture("media/startRP.png"));
  launchRP->setUseAlphaChannel();
  y1 = y2 +   bR; y2 = y1 +   bH; irr::gui::IGUIButton* launchMH    = device->getGUIEnvironment()->addButton(irr::core::rect<irr::s32>(x1,y1,x2,y2),0,MH_BUTTON,language.translate("startMH").c_str()); //i18n
  launchMH->setImage(driver->getTexture("media/startMH.png"));
  launchMH->setUseAlphaChannel();
  y1 = y2 + 3*bR; y2 = y1 +   bH; irr::gui::IGUIButton* launchINIBC = device->getGUIEnvironment()->addButton(irr::core::rect<irr::s32>(x1,y1,x2,y2),0,INI_BC_BUTTON,language.translate("startINIBC").c_str()); //i18n
  launchINIBC->setImage(driver->getTexture("media/settings.png"));
  launchINIBC->setUseAlphaChannel();
  y1 = y2 +   bR; y2 = y1 +   bH; irr::gui::IGUIButton* launchINIMC = device->getGUIEnvironment()->addButton(irr::core::rect<irr::s32>(x1,y1,x2,y2),0,INI_MC_BUTTON,language.translate("startINIMC").c_str()); //i18n
  launchINIMC->setImage(driver->getTexture("media/settings.png"));
  launchINIMC->setUseAlphaChannel();
  y1 = y2 +   bR; y2 = y1 +   bH; irr::gui::IGUIButton* launchINIRP = device->getGUIEnvironment()->addButton(irr::core::rect<irr::s32>(x1,y1,x2,y2),0,INI_RP_BUTTON,language.translate("startINIRP").c_str()); //i18n
  launchINIRP->setImage(driver->getTexture("media/settings.png"));
  launchINIRP->setUseAlphaChannel();
  y1 = y2 +   bR; y2 = y1 +   bH; irr::gui::IGUIButton* launchINIMH = device->getGUIEnvironment()->addButton(irr::core::rect<irr::s32>(x1,y1,x2,y2),0,INI_MH_BUTTON,language.translate("startINIMH").c_str()); //i18n
  launchINIMH->setImage(driver->getTexture("media/settings.png"));
  launchINIMH->setUseAlphaChannel();

  y1 = y2 + 3*bR; y2 = y1 +   bH; irr::gui::IGUIButton* launchDOC   = device->getGUIEnvironment()->addButton(irr::core::rect<irr::s32>(x1,y1,x2,y2),0,DOC_BUTTON,language.translate("startDOC").c_str()); //i18n
  launchDOC->setImage(driver->getTexture("media/startDOC.png"));
  launchDOC->setUseAlphaChannel();
  y1 = y2 +   bR; y2 = y1 +   bH; irr::gui::IGUIButton* launchCREDITS = device->getGUIEnvironment()->addButton(irr::core::rect<irr::s32>(x1,y1,x2,y2),0,CREDITS_BUTTON,language.translate("credits").c_str()); //i18n
  launchCREDITS->setImage(driver->getTexture("media/startDOC.png"));
  launchCREDITS->setUseAlphaChannel();
  y1 = y2 +   bR; y2 = y1 +   bH; irr::gui::IGUIButton* launchFOLDER= device->getGUIEnvironment()->addButton(irr::core::rect<irr::s32>(x1,y1,x2,y2),0,USER_BUTTON,language.translate("user").c_str()); //i18n
  launchFOLDER->setImage(driver->getTexture("media/user.png"));
  launchFOLDER->setUseAlphaChannel();
  y1 = y2 +   bR; y2 = y1 +   bH; irr::gui::IGUIButton* leave       = device->getGUIEnvironment()->addButton(irr::core::rect<irr::s32>(x1,y1,x2,y2),0,EXIT_BUTTON,language.translate("leave").c_str()); //i18n
  leave->setImage(driver->getTexture("media/leave.png"));
  leave->setUseAlphaChannel();

  std::string version = SHORTNAME + "_v" + VERSION;
  irr::core::stringw wVer(version.c_str());

  y1 = y2 + bR; y2 = y1 + bH;
  device->getGUIEnvironment()->addStaticText(wVer.c_str(), irr::core::rect<irr::s32>(165+wVer.size(), y1, x2, y2), true);
  device->getGUIEnvironment()->setFocus(launchBC);

  Receiver receiver(device, &language);
  device->setEventReceiver(&receiver);

#ifdef _WIN32
  if(0 != CHDIR("../bin/win/"))//Launch from builded sources
#else
    if(0 != chdir("../bin/linux/"))//Launch from builded sources
#endif
      {
	if(0 != CHDIR("/usr/bin"))//Launch from install
	  {
	    std::cout << "Bidge Commands not able to get binaries files" << std::endl;
	    exit(-1);
	  }
      }

  if(GETCWD(cwd, sizeof(cwd)) != NULL) printf("Launcher::Working Directory : %s\n", cwd);
      

  while (device->run()) {
    driver->beginScene(irr::video::ECBF_COLOR | irr::video::ECBF_DEPTH, irr::video::SColor(0, 200, 200, 200));
    device->getGUIEnvironment()->drawAll();
    driver->endScene();
    device->sleep(100);
  }

  return EXIT_FAILURE;
}
