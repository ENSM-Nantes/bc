/*   Bridge Command 5.0 Ship Simulator
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

#include "ScenarioChoice.hpp"
#include "StartupEventReceiver.hpp"
#include "Constants.hpp"
#include "IniFile.hpp"

#include "ExitMessage.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

//using namespace irr;

ScenarioChoice::ScenarioChoice(irr::IrrlichtDevice* device, Lang* language)
{
    this->language = language;
    this->device = device;
    gui = device->getGUIEnvironment();
}

void ScenarioChoice::chooseScenario(std::string& scenarioName, OperatingMode::Mode& mode, std::string scenarioPath, float fontScale)
{
    irr::video::IVideoDriver* driver = device->getVideoDriver();

    //Get list of scenarios, stored in scenarioList
    std::vector<std::string> scenarioList;
    std::vector<std::string> scenarioDescription;
    std::vector<std::string> scenarioShipsAndMap;
    getScenarioList(scenarioList, scenarioDescription, scenarioShipsAndMap, scenarioPath); //Populate list

    //Get screen width
    irr::u32 su = driver->getScreenSize().Width;
    irr::u32 sh = driver->getScreenSize().Height;

    //Make gui elements
    irr::core::stringw titleText(LONGNAME.c_str());
    irr::s32 titleFontSize = (irr::s32)(18 * fontScale + 0.5);
    if (titleFontSize > 22) {titleFontSize = 22;}
    std::string titleFontPath = "media/fonts/open-sans/open-sans-" + std::to_string(titleFontSize) + ".xml";
    irr::gui::IGUIFont* titleFont = gui->getFont(titleFontPath.c_str());
    irr::video::SColor titleColor(255,20,70,160);

    irr::core::dimension2d<irr::u32> titleDimensions = titleFont->getDimension(titleText.c_str());
    irr::gui::IGUIStaticText* title = gui->addStaticText(titleText.c_str(),irr::core::rect<irr::s32>((su-titleDimensions.Width)/2, 0.017*sh, (su+titleDimensions.Width)/2, 0.09*sh));

    title->setOverrideColor(titleColor);
    if (titleFont != 0) {title->setOverrideFont(titleFont);}
    title->setTextAlignment(irr::gui::EGUIA_CENTER,irr::gui::EGUIA_CENTER);

    irr::s32 headerFontSize = (irr::s32)(20 * fontScale + 0.5);
    if (headerFontSize > 22) {headerFontSize = 22;}
    std::string headerFontPath = "media/fonts/open-sans/open-sans-" + std::to_string(headerFontSize) + ".xml";
    irr::gui::IGUIFont* headerFont = gui->getFont(headerFontPath.c_str());
    irr::video::SColor headerColor(255,20,70,160);

    irr::gui::IGUIStaticText* instruction = gui->addStaticText(language->translate("scnChoose").c_str(),irr::core::rect<irr::s32>(0.075*su,0.13*sh,0.4625*su, 0.17*sh));
    instruction->setOverrideColor(headerColor);
    if (headerFont != 0) {instruction->setOverrideFont(headerFont);}
    irr::gui::IGUIListBox* scenarioListBox = gui->addListBox(irr::core::rect<irr::s32>(0.075*su,0.17*sh,0.4625*su,0.50*sh),0,GUI_ID_SCENARIO_LISTBOX);
    irr::gui::IGUIStaticText* description = gui->addStaticText(L"",irr::core::rect<irr::s32>(0.075*su,0.51*sh,0.4625*su,0.62*sh));
    //Box to the right of the window, listing the map and ships used by the selected scenario
    irr::gui::IGUIStaticText* shipsMapTitle = gui->addStaticText(language->translate("scenarioDetails").c_str(),irr::core::rect<irr::s32>(0.5375*su,0.13*sh,0.925*su, 0.17*sh));
    shipsMapTitle->setOverrideColor(headerColor);
    if (headerFont != 0) {shipsMapTitle->setOverrideFont(headerFont);}
    irr::gui::IGUIStaticText* shipsMapText = gui->addStaticText(L"",irr::core::rect<irr::s32>(0.5375*su,0.17*sh,0.925*su,0.50*sh),true,true);
    //Launch button sits in the bottom-right corner of the window, with the same 0.075*su
    //margin from the window edge as the scenario details box above it. Moved up from the
    //very bottom edge to leave room for the version/build text below it.
    irr::gui::IGUIButton* okButton = gui->addButton(irr::core::rect<irr::s32>(0.645*su,0.82*sh,0.925*su,0.90*sh),0,GUI_ID_OK_BUTTON,language->translate("launchScenario").c_str());
    //Exit button sits in the bottom-left corner of the window, for quitting without choosing
    //a scenario - same 0.075*su margin from the window edge as the scenario list above it
    irr::gui::IGUIButton* exitButton = gui->addButton(irr::core::rect<irr::s32>(0.075*su,0.82*sh,0.255*su,0.90*sh),0,GUI_ID_EXIT_BUTTON,language->translate("exit").c_str());

    irr::core::stringw secondaryLabel = language->translate("secondary");
    irr::u32 secondaryLabelWidth = gui->getSkin()->getFont()->getDimension(secondaryLabel.c_str()).Width;
    irr::gui::IGUIStaticText* secondaryText = gui->addStaticText(secondaryLabel.c_str(),irr::core::rect<irr::s32>(0.075*su,0.64*sh,0.075*su+secondaryLabelWidth, 0.68*sh));
    secondaryText->setTextAlignment(irr::gui::EGUIA_UPPERLEFT, irr::gui::EGUIA_CENTER);
    irr::gui::IGUICheckBox* secondaryCheckbox = gui->addCheckBox(false,irr::core::rect<irr::s32>(0.075*su+secondaryLabelWidth+0.01*su,0.64*sh,0.075*su+secondaryLabelWidth+0.03*su,0.68*sh),0,GUI_ID_SECONDARY_CHECKBOX);

    irr::core::stringw multiplayerLabel = language->translate("multiplayer");
    irr::u32 multiplayerLabelWidth = gui->getSkin()->getFont()->getDimension(multiplayerLabel.c_str()).Width;
    irr::gui::IGUIStaticText* multiplayerText = gui->addStaticText(multiplayerLabel.c_str(),irr::core::rect<irr::s32>(0.075*su,0.69*sh,0.075*su+multiplayerLabelWidth, 0.73*sh));
    multiplayerText->setTextAlignment(irr::gui::EGUIA_UPPERLEFT, irr::gui::EGUIA_CENTER);
    irr::gui::IGUICheckBox* multiplayerCheckbox = gui->addCheckBox(false,irr::core::rect<irr::s32>(0.075*su+multiplayerLabelWidth+0.01*su,0.69*sh,0.075*su+multiplayerLabelWidth+0.03*su,0.73*sh),0,GUI_ID_MULTIPLAYER_CHECKBOX);


    irr::core::stringw versionText(L"Version : ");
    versionText.append(VERSION.c_str());
    versionText.append(L"\nBuild: ");
    versionText.append(irr::core::stringw(__DATE__));
    versionText.append(L" ");
    versionText.append(irr::core::stringw(__TIME__));

    //Version/build info, bottom-right corner of the window, below the launch button
    irr::gui::IGUIStaticText* version = gui->addStaticText(versionText.c_str(),irr::core::rect<irr::s32>(0.5375*su,0.90*sh,0.925*su,0.98*sh));
    version->setTextAlignment(irr::gui::EGUIA_LOWERRIGHT, irr::gui::EGUIA_LOWERRIGHT);

    //add credits text
    //irr::gui::IGUIStaticText* creditsText = gui->addStaticText((getCredits()).c_str(),irr::core::rect<irr::s32>(0.35*su,0.35*sh,0.95*su, 0.95*sh),true);

    //Add scenarios to list box
    for (std::vector<std::string>::iterator it = scenarioList.begin(); it != scenarioList.end(); ++it) {
        scenarioListBox->addItem(irr::core::stringw(it->c_str()).c_str()); //Note - odd conversion from char* to wchar*!
    }
    //select first one if possible
    if (scenarioListBox->getItemCount()>0) {
        scenarioListBox->setSelected(0);
    }
    //select list box as active, so user can use up/down arrows without needing to click
    gui->setFocus(scenarioListBox);

    //Flush old key/clicks etc, with a 0.2s pause
    device->sleep(200);
    device->clearSystemMessages();

    //Link to our event receiver
    StartupEventReceiver startupReceiver(scenarioListBox,instruction,secondaryCheckbox,multiplayerCheckbox,description,GUI_ID_SCENARIO_LISTBOX,GUI_ID_OK_BUTTON,GUI_ID_SECONDARY_CHECKBOX,GUI_ID_MULTIPLAYER_CHECKBOX,GUI_ID_EXIT_BUTTON, device);
    irr::IEventReceiver* oldReceiver = device->getEventReceiver();
    device->setEventReceiver(&startupReceiver);

    irr::s32 descriptionScenario = -1; //Which scenario we are showing the description text for

    while(device->run() && startupReceiver.getScenarioSelected()==-1) {
            //Event receiver will set Scenario Selected, so we just loop here until that happens
            driver->beginScene(irr::video::ECBF_COLOR|irr::video::ECBF_DEPTH, irr::video::SColor(0,200,200,200));
            
            //Set the 'description' text here
            irr::s32 currentSelection = scenarioListBox->getSelected();
            if (currentSelection!=descriptionScenario) {
                //Update the description text
                description->setText(L"");
                if (scenarioDescription.size() > currentSelection && currentSelection>=0) {
                    description->setText(irr::core::stringw(scenarioDescription.at(currentSelection).c_str()).c_str());
                }
                //Update the map/ships text
                shipsMapText->setText(L"");
                if (scenarioShipsAndMap.size() > currentSelection && currentSelection>=0) {
                    shipsMapText->setText(irr::core::stringw(scenarioShipsAndMap.at(currentSelection).c_str()).c_str());
                }
                currentSelection = descriptionScenario;
            }

            //Hide the map/ships box along with the other scenario-selection controls
            //when secondary or multiplayer mode is chosen instead
            bool scenarioDetailsVisible = !(secondaryCheckbox->isChecked() || multiplayerCheckbox->isChecked());
            shipsMapTitle->setVisible(scenarioDetailsVisible);
            shipsMapText->setVisible(scenarioDetailsVisible);

            gui->drawAll();
            driver->endScene();
    }



    //Get name of selected scenario
    if (startupReceiver.getScenarioSelected()<0 || startupReceiver.getScenarioSelected() >= (irr::s32)scenarioList.size()) {
		ExitMessage::exitWithMessage("No scenario selected.");
    }

    //Check if 'secondary' mode is selected
    if(secondaryCheckbox->isChecked()) {
        mode = OperatingMode::Secondary;
    } else if (multiplayerCheckbox->isChecked()) {
        mode = OperatingMode::Multiplayer;
    } else {
        mode = OperatingMode::Normal;
    }

    if (mode == OperatingMode::Normal) {
        //Use selected scenario - don't need this in secondary mode
        scenarioName = scenarioList[startupReceiver.getScenarioSelected()]; //scenarioName is a pass by reference return value
    }

    //Clean up
    scenarioListBox->remove(); scenarioListBox = 0;
    okButton->remove(); okButton = 0;
    exitButton->remove(); exitButton = 0;
    title->remove(); title = 0;
    instruction->remove(); instruction=0;
    secondaryText->remove(); secondaryText=0;
    secondaryCheckbox->remove(); secondaryCheckbox=0;
    multiplayerCheckbox->remove();multiplayerCheckbox=0;
    multiplayerText->remove();multiplayerText=0;
    description->remove();description=0;
    shipsMapTitle->remove();shipsMapTitle=0;
    shipsMapText->remove();shipsMapText=0;
    version->remove();version=0;
    //creditsText->remove(); creditsText=0;
    device->setEventReceiver(oldReceiver); //Remove link to startup event receiver, as this will be destroyed, and return to what we were using

    return;
}

void ScenarioChoice::getScenarioList(std::vector<std::string>&scenarioList, std::vector<std::string>&scenarioDescription, std::vector<std::string>&scenarioShipsAndMap, std::string scenarioPath) {

	irr::io::IFileSystem* fileSystem = device->getFileSystem();
	if (fileSystem==0) {
        ExitMessage::exitWithMessage("Could not get file system access.");
    }
    //store current dir
	irr::io::path cwd = fileSystem->getWorkingDirectory();

    //change to scenario dir
    if (!fileSystem->changeWorkingDirectoryTo(scenarioPath.c_str())) {
        ExitMessage::exitWithMessage("Could not get change working directory to scenario directory.");
    }

    irr::io::IFileList* fileList = fileSystem->createFileList();
    if (fileList==0) {
		ExitMessage::exitWithMessage("Could not get file list for secenarios.");
    }

    //List here
    for (irr::u32 i=0;i<fileList->getFileCount();i++) {
        if (fileList->isDirectory(i)) {
            const irr::io::path& fileName = fileList->getFileName(i);
            if (fileName.findFirst('.')!=0) { //Check it doesn't start with '.' (., .., or hidden)

                //Don't include scenarios ending in _mp (Multiplayer)
                //Check if name ends with "_mp" for multiplayer:
                bool multiplayerScenario = false;
                if (fileName.size() >= 3) {
                    const irr::io::path endChars = fileName.subString(fileName.size()-3,3,true);
                    if (endChars == irr::io::path("_mp")) {
                        multiplayerScenario = true;
                    }
                }

                //Add scenario to the list
                if (!multiplayerScenario) {
                    scenarioList.push_back(fileName.c_str());

                    //Try reading description.ini if it exists
                    irr::io::path descriptionFilePath = fileName;
                    irr::io::path descriptionFilename = descriptionFilePath.append("/description.ini");
                    std::ifstream descriptionStream (descriptionFilename.c_str());
                    //Set UTF-8 on Linux/OSX etc
                    #ifndef _WIN32
                        try {
                    #  ifdef __APPLE__
                            char* thisLocale = setlocale(LC_ALL, "");
                            if (thisLocale) {
                                descriptionStream.imbue(std::locale(thisLocale));
                            }
                    #  else
                            descriptionStream.imbue(std::locale("en_US.UTF8"));
                    #  endif
                        } catch (const std::runtime_error& runtimeError) {
                            descriptionStream.imbue(std::locale(""));
                        }
                    #endif

                    std::string descriptionLines="";
                    if (descriptionStream.is_open()) {
                        std::string descriptionLine;
                        while ( std::getline (descriptionStream,descriptionLine) )
                        {
                            descriptionLines.append(descriptionLine);
                            descriptionLines.append("\n");

                        }
                        descriptionStream.close();
                    }
                    scenarioDescription.push_back(descriptionLines); //Add even if empty

                    //Read the map (world) name and the ships present, from environment.ini/ownship.ini/othership.ini
                    irr::io::path envFilePath = fileName;
                    std::string envFilename = envFilePath.append("/environment.ini").c_str();
                    irr::io::path ownshipFilePath = fileName;
                    std::string ownshipFilename = ownshipFilePath.append("/ownship.ini").c_str();
                    irr::io::path othershipFilePath = fileName;
                    std::string othershipFilename = othershipFilePath.append("/othership.ini").c_str();

                    std::string mapName = IniFile::iniFileToString(envFilename, "Setting");
                    std::string ownShipName = IniFile::iniFileToString(ownshipFilename, "ShipName");
                    irr::u32 otherShipCount = IniFile::iniFileTou32(othershipFilename, "Number");

                    //Format the scenario's start date/time (StartTime is in decimal hours, e.g. 10.5 = 10:30)
                    irr::f32 startTime = IniFile::iniFileTof32(envFilename, "StartTime");
                    irr::u32 startDay = IniFile::iniFileTou32(envFilename, "StartDay");
                    irr::u32 startMonth = IniFile::iniFileTou32(envFilename, "StartMonth");
                    irr::u32 startYear = IniFile::iniFileTou32(envFilename, "StartYear");
                    irr::u32 startHour = (irr::u32)startTime;
                    irr::u32 startMinute = (irr::u32)((startTime - startHour) * 60 + 0.5f);

                    std::ostringstream dateTimeStream;
                    dateTimeStream << std::setfill('0') << std::setw(2) << startDay << "/"
                                   << std::setw(2) << startMonth << "/" << startYear << " "
                                   << std::setw(2) << startHour << ":" << std::setw(2) << startMinute;

                    std::string shipsAndMap = "Date/time: " + dateTimeStream.str() + "\n\nMap: " + mapName + "\n\nShips:";
                    if (ownShipName.size() > 0) {
                        shipsAndMap += "\n> " + ownShipName + " (own ship)";
                    }
                    for (irr::u32 s = 1; s <= otherShipCount; s++) {
                        std::string otherShipType = IniFile::iniFileToString(othershipFilename, IniFile::enumerate1("Type", s));
                        if (otherShipType.size() > 0) {
                            shipsAndMap += "\n> " + otherShipType;
                        }
                    }
                    scenarioShipsAndMap.push_back(shipsAndMap);

                }

            }
        }
    }

    //change back
    if (!fileSystem->changeWorkingDirectoryTo(cwd)) {
        ExitMessage::exitWithMessage("Can't return to normal working directory.");
    }
    fileList->drop();
}
