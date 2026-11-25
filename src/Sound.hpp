/*   Bridge Command 5.0 Ship Simulator
     Copyright (C) 2018 James Packer

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

//Based on sample code from https://github.com/hosackm/wavplayer/blob/master/main.c

#ifndef __SOUND_HPP_INCLUDED__
#define __SOUND_HPP_INCLUDED__

#define FRAMES_PER_BUFFER   (512)

#ifdef WITH_SOUND
#include <sndfile.h>
#include <portaudio.h>
#endif // WITH_SOUND
#include <string.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <atomic>

class Sound
{
public:

  Sound();
  ~Sound();
  void load(std::string aBasePathx);
  void StartSound();
  void setVolumeWave(float vol);
  void setVolumeEngine(float vol);
  void setVolumeHorn(float vol);
  void setVolumeAlarm(float vol);
  float getVolumeWave() const;
  float getVolumeEngine() const;
  float getVolumeHorn() const;
  float getVolumeAlarm() const;
  void setAlarm(bool alarmState);
  void startHorn();
  void endHorn();
  void startBell();
  void endBell();
  void startGong();
  void endGong();
  void setOwnShipSounds(void);

  
#ifdef WITH_SOUND
private:

  typedef struct
  {
    SNDFILE     *fileWave;
    SNDFILE     *fileEngine;
    SNDFILE     *fileHorn;
    SNDFILE     *fileAlarm;
    SNDFILE     *fileBell;
    SNDFILE     *fileGong;
    SF_INFO      infoWave;
    SF_INFO      infoEngine;
    SF_INFO      infoHorn;
    SF_INFO      infoAlarm;
    SF_INFO      infoBell;
    SF_INFO      infoGong;
  } callback_data_s;

  std::string mBasePath;
  std::string mEnginePath;
  std::string mBwavePath;
  std::string mHornPath;
  std::string mAlarmPath;
  std::string mBellPath;
  std::string mGongPath;
  static float hornVolume;
  static float waveVolume;
  static float engineVolume;
  static float alarmVolume;
  static float bellVolume;
  static float gongVolume;

  bool soundLoaded;
  static bool waveSoundLoaded;
  static bool hornSoundLoaded;
  static bool alarmSoundLoaded;
  static bool bellSoundLoaded;
  static bool gongSoundLoaded;
  static std::atomic<bool> bellRestart;
  static std::atomic<bool> gongRestart;

  bool openOptionalSound(const std::string& aPath, const std::string& aName, SNDFILE*& aFile, SF_INFO& aInfo);

  static void readLoop(SNDFILE* aFile, std::atomic<bool>& aRestart, float* aBuffer, sf_count_t aCount)
  {
    if (aRestart.exchange(false))
      sf_seek(aFile, 0, SEEK_SET);
    sf_count_t done = sf_read_float(aFile, aBuffer, aCount);
    if (done < aCount)
      {
	sf_seek(aFile, 0, SEEK_SET);
	done += sf_read_float(aFile, aBuffer + done, aCount - done);
      }
    if (done < aCount)
      memset(aBuffer + done, 0, sizeof(float) * (aCount - done));
  }
  PaError portAudioError;
  PaStream *stream;
  //SNDFILE *file;
  callback_data_s data;

  static int callback
  (const void                     *input
   , void                           *output
   , unsigned long                   frameCount
   , const PaStreamCallbackTimeInfo *timeInfo
   , PaStreamCallbackFlags           statusFlags
   , void                           *userData
   )
  {
    float           *out;
    callback_data_s *p_data = (callback_data_s*)userData;
    sf_count_t       num_read;

    out = (float*)output;
    p_data = (callback_data_s*)userData;

    //Note that we've ensured already that channels are the same for all files, if both waveSoundLoaded and hornSoundLoaded are true

    /* clear output buffer */
    memset(out, 0, sizeof(float) * frameCount * p_data->infoEngine.channels);

    //Create four buffers, for wave, engine, horn and alarm
    std::vector<float> engineBuffer(sizeof(float) * frameCount * p_data->infoEngine.channels);
    std::vector<float> waveBuffer(sizeof(float) * frameCount * p_data->infoEngine.channels);
    std::vector<float> hornBuffer(sizeof(float) * frameCount * p_data->infoEngine.channels);
    std::vector<float> alarmBuffer(sizeof(float) * frameCount * p_data->infoEngine.channels);
    std::vector<float> bellBuffer(sizeof(float) * frameCount * p_data->infoEngine.channels);
    std::vector<float> gongBuffer(sizeof(float) * frameCount * p_data->infoEngine.channels);

    /* read into buffers */
    num_read = sf_read_float(p_data->fileEngine, engineBuffer.data(), frameCount * p_data->infoEngine.channels);
    /*  If we couldn't read a full frameCount of samples we've reached EOF */
    //Try to restart
    if (num_read < frameCount)
      {

	sf_count_t seekLocation = sf_seek(p_data->fileEngine, 0, SEEK_SET);
	if (seekLocation == -1) {
	  return paComplete;
	}

	//Read again
	/* read directly into output buffer */
	num_read = sf_read_float(p_data->fileEngine, engineBuffer.data(), frameCount * p_data->infoEngine.channels);

	/*  If we couldn't read a full frameCount of samples we've reached EOF */
	if (num_read < frameCount) {
	  return paComplete;
	}
      }

    if (waveSoundLoaded) {
      num_read = sf_read_float(p_data->fileWave, waveBuffer.data(), frameCount * p_data->infoEngine.channels);
      /*  If we couldn't read a full frameCount of samples we've reached EOF */
      //Try to restart
      if (num_read < frameCount)
	{

	  sf_count_t seekLocation = sf_seek(p_data->fileWave, 0, SEEK_SET);
	  if (seekLocation == -1) {
	    return paComplete;
	  }

	  //Read again
	  /* read directly into output buffer */
	  num_read = sf_read_float(p_data->fileWave, waveBuffer.data(), frameCount * p_data->infoEngine.channels);

	  /*  If we couldn't read a full frameCount of samples we've reached EOF */
	  if (num_read < frameCount) {
	    return paComplete;
	  }
	}
    }

    if (hornSoundLoaded) {
      num_read = sf_read_float(p_data->fileHorn, hornBuffer.data(), frameCount * p_data->infoEngine.channels);
      /*  If we couldn't read a full frameCount of samples we've reached EOF */
      //Try to restart
      if (num_read < frameCount)
	{

	  sf_count_t seekLocation = sf_seek(p_data->fileHorn, 0, SEEK_SET);
	  if (seekLocation == -1) {
	    return paComplete;
	  }

	  //Read again
	  /* read directly into output buffer */
	  num_read = sf_read_float(p_data->fileHorn, hornBuffer.data(), frameCount * p_data->infoEngine.channels);

	  /*  If we couldn't read a full frameCount of samples we've reached EOF */
	  if (num_read < frameCount) {
	    return paComplete;
	  }
	}
    }

    if (alarmSoundLoaded) {
      num_read = sf_read_float(p_data->fileAlarm, alarmBuffer.data(), frameCount * p_data->infoEngine.channels);
      /*  If we couldn't read a full frameCount of samples we've reached EOF */
      //Try to restart
      if (num_read < frameCount)
	{

	  sf_count_t seekLocation = sf_seek(p_data->fileAlarm, 0, SEEK_SET);
	  if (seekLocation == -1) {
	    return paComplete;
	  }

	  //Read again
	  /* read directly into output buffer */
	  num_read = sf_read_float(p_data->fileAlarm, alarmBuffer.data(), frameCount * p_data->infoEngine.channels);

	  /*  If we couldn't read a full frameCount of samples we've reached EOF */
	  if (num_read < frameCount) {
	    return paComplete;
	  }
	}
    }

    if (bellSoundLoaded)
      readLoop(p_data->fileBell, bellRestart, bellBuffer.data(), frameCount * p_data->infoEngine.channels);

    if (gongSoundLoaded)
      readLoop(p_data->fileGong, gongRestart, gongBuffer.data(), frameCount * p_data->infoEngine.channels);

    //Copy into output buffer, with mixing
    for (int i = 0; i < frameCount * p_data->infoWave.channels; i++) {
      out[i] = engineVolume*engineBuffer[i] * 0.33;
      if (waveSoundLoaded) {
	out[i] += waveVolume*waveBuffer[i] * 0.33;
      }
      if (hornSoundLoaded) {
	out[i] += hornVolume*hornBuffer[i] * 0.33;
      }
      if (alarmSoundLoaded) {
	out[i] += alarmVolume*alarmBuffer[i] * 0.33;
      }
      if (bellSoundLoaded) {
	out[i] += bellVolume*bellBuffer[i] * 0.33;
      }
      if (gongSoundLoaded) {
	out[i] += gongVolume*gongBuffer[i] * 0.33;
      }
    }

    return paContinue;
  }

#endif // WITH_SOUND

};

#endif
