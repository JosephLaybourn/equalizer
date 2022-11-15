// Equalizer2Demo.cpp
// -- bass/treble control, variable Equalizer2 point
// jsh 9/19
//
// usage:
//  Equalizer2Demo [<file>]
// where:
//   <file> -- (optional) name of a WAVE file
// note:
//   if no file is specified, white noise is used
//
// to compile:
//   cl /EHsc Equalizer2Demo.cpp LowPass.cpp HighPass.cpp\
//      Control.cpp portaudio_x86.lib user32.lib

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <cmath>
#include <cstdlib>
#include <portaudio.h>
#include "Control.h"
#include "LowPass.h"
#include "HighPass.h"
using namespace std;


class Equalizer2 : public Control {
  public:
    Equalizer2(float R, short *s, unsigned n, bool st);
    static int OnWrite(const void*,void*,unsigned long,
                       const PaStreamCallbackTimeInfo*,
                       PaStreamCallbackFlags,void*);
  private:
    void valueChanged(unsigned,int);
    short *samples;
    unsigned count,
             index;
    bool stereo;
    float rate,
          gain_LP, gain_HP;
    LowPass1 lp1_left, lp1_right;
    LowPass2 lp2_left, lp2_right;
    HighPass1 hp1_left, hp1_right;
    HighPass2 hp2_left, hp2_right;
    FrequencyFilter *lp_left_ptr, *lp_right_ptr,
                    *hp_left_ptr, *hp_right_ptr;
};


Equalizer2::Equalizer2(float R, short *s, unsigned n, bool st)
    : Control(4,"2-Band Equalizer Demo"),
      samples(s),
      count(n),
      index(0),
      stereo(st),
      rate(R),
      gain_LP(1), gain_HP(1),
      lp1_left(200,R), lp1_right(200,R),
      lp2_left(200,R), lp2_right(200,R),
      hp1_left(200,R), hp1_right(200,R),
      hp2_left(200,R), hp2_right(200,R),
      lp_left_ptr(&lp1_left), lp_right_ptr(&lp1_right),
      hp_left_ptr(&hp1_left), hp_right_ptr(&hp2_right) {
  // 0: Equalizer2 point
  setLabel(0,"Cutoff freqeuncy: 200 Hz");
  setRange(0,0,1000); // 20-2000 Hz (log scale)
  setValue(0,500);    // 200 Hz
  // 1: bass gain
  setLabel(1,"Bass gain: 0 dB");
  setRange(1,0,1000); // -24 - 24 dB
  setValue(1,500);    // 0 dB
  // 2: treble gain
  setLabel(2,"Treble gain: 0 dB");
  setRange(2,0,1000); // -24 - 24 dB
  setValue(2,500);    // 0 dB
  // 3: rolloff
  setLabel(3,"Filter rolloff: 3 dB/oct");
  setRange(3,0,1);   // 0=3dB/oct, 1=6dB/oct
  setValue(3,0);     // 3dB/oct
}


void Equalizer2::valueChanged(unsigned index, int value) {
  const float TWOPI = 8.0f*atan(1.0f);
  stringstream ss;
  switch (index) {

    case 0: // Equalizer2 freq
      { float f = 20 * pow(10,0.002f*value);
        lp1_left.setFrequency(f);  lp1_right.setFrequency(f);
        lp2_left.setFrequency(f);  lp2_right.setFrequency(f);
        hp1_left.setFrequency(f);  hp1_right.setFrequency(f);
        hp2_left.setFrequency(f);  hp2_right.setFrequency(f);
        ss << "Cutoff frequency: " << fixed
           << setprecision(1) << f << " Hz";
      }
      break;

    case 1: // bass gain
      { float dB = -24+0.048f*value;
        gain_LP = pow(10,dB/20.0f);
        ss << "Bass gain: " << fixed << setprecision(1)
           << dB << " dB";
      }
      break;

    case 2: // treble gain
      { float dB = -24+0.048f*value;
        gain_HP = pow(10,dB/20.0f);
        ss << "Treble gain: " << fixed << setprecision(1)
           << dB << " dB";
      }
      break;

    case 3: // filter rolloff
      { lp_left_ptr = (value == 0) ? (FrequencyFilter*)&lp1_left 
                                   : (FrequencyFilter*)&lp2_left;
        lp_right_ptr = (value == 0) ? (FrequencyFilter*)&lp1_right
                                    : (FrequencyFilter*)&lp2_right;
        hp_left_ptr = (value == 0) ? (FrequencyFilter*)&hp1_left
                                   : (FrequencyFilter*)&hp2_left;
        hp_right_ptr = (value == 0) ? (FrequencyFilter*)&hp1_right
                                    : (FrequencyFilter*)&hp2_right;
        ss << "Filter rolloff: " << (value == 0 ? "6" : "12")
           << " dB/oct";
      }

   }
   setLabel(index,ss.str().c_str());
}


int Equalizer2::OnWrite(const void *vin, void *vout, unsigned long frames,
                       const PaStreamCallbackTimeInfo *tinfo,
                       PaStreamCallbackFlags flags, void *user) {
  const float IMAX = 1.0f/float(1<<15);

  Equalizer2& cr = *reinterpret_cast<Equalizer2*>(user);
  float *out = reinterpret_cast<float*>(vout);
              
  if (cr.stereo) {
    for (unsigned long i=0; i < frames; ++i) {
      float L = IMAX * cr.samples[2*cr.index],
            R = IMAX * cr.samples[2*cr.index+1];
      out[2*i] =   cr.gain_LP * (*cr.lp_left_ptr)(L)
                 - cr.gain_HP * (*cr.hp_left_ptr)(L);
      out[2*i+1] =   cr.gain_LP * (*cr.lp_right_ptr)(R)
                   - cr.gain_HP * (*cr.hp_right_ptr)(R);
      cr.index = (cr.index + 1) % cr.count;
    }
  }

  else {
    for (unsigned long i=0; i < frames; ++i) {
      float x = IMAX * cr.samples[cr.index];         // invert the HP gain
      out[i] =   cr.gain_LP * (*cr.lp_left_ptr)(x)   // to avoid destructive
               - cr.gain_HP * (*cr.hp_left_ptr)(x);  // interference in 2nd
      cr.index = (cr.index + 1) % cr.count;          // order filter
    }
  }

  return paContinue;
}


int main(int argc, char *argv[]) {
  const int DEF_RATE = 44100;
  const float MAX = float((1<<15) - 1);

  if (argc != 1 && argc != 2)
    return 0;

  short *samples;
  unsigned channels,
           rate,
           count,
           size;

  // read in input file
  if (argc == 2) {
    fstream in(argv[1],ios_base::binary|ios_base::in);
    char header[44];
    in.read(header,44);
    channels = *reinterpret_cast<unsigned short*>(header+22),
    rate = *reinterpret_cast<unsigned*>(header+24),
    size = *reinterpret_cast<unsigned*>(header+40),
    count = size/(2*channels);
    samples = new short[channels*count];
    in.read(reinterpret_cast<char*>(samples),size);
    if (!in) {
      cout << "bad file: " << argv[1] << endl;
      return 0;
    }
  }

  // otherwise, generate white noise
  else {
    channels = 1;
    rate = DEF_RATE;
    count = rate;
    size = 2*count;
    samples = new short[count];
    for (int i=0; i < count; ++i)
      samples[i] = MAX * (1 - 2*float(rand())/float(RAND_MAX));
  }

  Equalizer2 Equalizer2(rate,samples,count,channels==2);

  Pa_Initialize();
  PaStreamParameters input_params;
  input_params.device = Pa_GetDefaultOutputDevice();
  input_params.channelCount = channels;
  input_params.sampleFormat = paFloat32;
  input_params.suggestedLatency = 2*Pa_GetDeviceInfo(input_params.device)
                                    ->defaultLowOutputLatency;
  input_params.hostApiSpecificStreamInfo = 0;

  PaStream *stream;
  Pa_OpenStream(&stream,0,&input_params,rate,0,0,
                Equalizer2::OnWrite,&Equalizer2);
  Pa_StartStream(stream);
  Equalizer2.show();

  cin.get();

  Equalizer2.show(false);
  Pa_StopStream(stream);
  Pa_CloseStream(stream);
  Pa_Terminate();

  delete[] samples;
  return 0;
}

