/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                          Sound.c                        **/
/**                                                         **/
/** This file file implements core part of the sound API    **/
/** and functions needed to log soundtrack into a MIDI      **/
/** file. See Sound.h for declarations.                     **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-2002                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#include "Sound.h"

#include <stdio.h>

struct SndDriverStruct SndDriver =
{
  (void (*)(int,int))0,
  (void (*)(int,int))0,
  (void (*)(int,int))0,
  (void (*)(int,int,int))0,
  (void (*)(int,signed char *,int,int))0
};

static struct { unsigned char Note;unsigned short Wheel; } Freqs[4096] =
{
#include "MIDIFreq.h"
};

/** Sound() **************************************************/
/** Generate sound of given frequency (Hz) and volume       **/
/** (0..255) via given channel. Setting Freq=0 or Volume=0  **/
/** turns sound off.                                        **/
/*************************************************************/
void Sound(int Channel,int Freq,int Volume)
{
  if(Channel<0) return;
  Freq   = Freq<0? 0:Freq;
  Volume = Volume<0? 0:Volume>255? 255:Volume;

  /* Call sound driver if present */
  if(SndDriver.Sound) (*SndDriver.Sound)(Channel,Freq,Volume);
}

/** Drum() ***************************************************/
/** Hit a drum of given type with given force (0..255).     **/
/** MIDI drums can be used by ORing their numbers with      **/
/** SND_MIDI.                                               **/
/*************************************************************/
void Drum(int Type,int Force)
{
  Force = Force<0? 0:Force>255? 255:Force;

  if(SndDriver.Drum) (*SndDriver.Drum)(Type,Force);
}

/** SetSound() ***********************************************/
/** Set sound type at a given channel. MIDI instruments can **/
/** be set directly by ORing their numbers with SND_MIDI.   **/
/*************************************************************/
void SetSound(int Channel,int Type)
{
  if(Channel<0) return;

  if(SndDriver.SetSound) (*SndDriver.SetSound)(Channel,Type);
}

/** SetChannels() ********************************************/
/** Set master volume (0..255) and switch channels on/off.  **/
/** Each channel N has corresponding bit 2^N in Switch. Set **/
/** or reset this bit to turn the channel on or off.        **/ 
/*************************************************************/
void SetChannels(int Volume,int Switch)
{
  Volume = Volume<0? 0:Volume>255? 255:Volume;

  if(SndDriver.SetChannels) (*SndDriver.SetChannels)(Volume,Switch);
}

/** SetWave() ************************************************/
/** Set waveform for a given channel. The channel will be   **/
/** marked with sound type SND_WAVE. Set Rate=0 if you want **/
/** waveform to be an instrument or set it to the waveform  **/
/** own playback rate.                                      **/
/*************************************************************/
void SetWave(int Channel,signed char *Data,int Length,int Rate)
{
  if((Channel<0)||(Length<=0)) return;

  if(SndDriver.SetWave) (*SndDriver.SetWave)(Channel,Data,Length,Rate);
}
