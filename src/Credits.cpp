#include "Credits.hpp"

irr::core::stringw getCredits(){

  irr::core::stringw creditsString(L"Bridge Command is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License version 2 as published by the Free Software Foundation.\n\n");
  creditsString.append(L"Bridge Command  is distributed  in the  hope that  it will  be useful, but WITHOUT ANY WARRANTY; without even the implied  warranty of  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.\n\n");
  creditsString.append(L"In memory of Sergio Fuentes, who provided many useful suggestions for the program's development.\n\n");
  creditsString.append(L"Many thanks to those who have made their models available for use in Bridge Command:\n");
  creditsString.append(L"> Juergen Klemp\n");
  creditsString.append(L"> Simon D Richardson\n");
  creditsString.append(L"> Jason Simpson\n");
  creditsString.append(L"> Ragnar\n");
  creditsString.append(L"> Thierry Videlaine\n");
  creditsString.append(L"> NETC (Naval Education and Training Command)\n");
  creditsString.append(L"> Sky image from 0ptikz\n\n");
  creditsString.append(L"Many thanks to Ken Trethewey for making his images of the Eddystone lighthouse available.\n\n");

  creditsString.append(L"Many thanks to contributors including David Elir Evans, Antoine Saillard, Konrad Wolsing, Jan Bauer, AndreySSH, Manfred, ceeac.\n\n");

  creditsString.append(L"Bridge Command uses the Irrlicht Engine, the ENet networking library, ASIO, PortAudio, water based on Keith Lantz FFT water implementation, RealisticWaterSceneNode by elvman, AIS Parser by Brian C. Lane, and the Serial library by William Woodall. Bridge Command depends on libsndfile, which is released under the GNU Lesser General Public License version 2.1 or 3.\n\n");

  creditsString.append(L"The Irrlicht Engine is based in part on the work of the Independent JPEG Group, the zlib, and libpng.");

  return creditsString;
}
