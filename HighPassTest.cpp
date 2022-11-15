// HighPassTest.cpp
// -- test of high pass filter
// cs246 9/19

#include <algorithm>
#include <fstream>
#include "HighPass.h"
using namespace std;


class Frand {
  public:
    Frand(void);
    float operator()(float a=0, float b=1);
  private:
    int value;
};


Frand::Frand(void) : value(1031) {
}
    

float Frand::operator()(float a, float b) {
  const int A = 2153,
            B = 4889,
            C = 7919;
  float x = a + (b-a)*float(value)/float(C);
  value = (A*value + B)%C;
  return x;
}


int main(void) {
  const unsigned RATE = 44100;
  const float MAX = float((1<<15)-1);
  Frand pseudorand;

  // pseudo-random noise
  unsigned count = 4*RATE;
  short *samples = new short[2*count];
  for (unsigned i=0; i < 2*count; ++i)
    samples[i] = MAX * pseudorand(-1,1);

  // 1st second: cutoff at 200 Hz
  HighPass1 hp1_1(200,RATE);
  HighPass2 hp1_2(200,RATE);
  for (unsigned i=0; i < RATE; ++i) {
    float y_L = hp1_1(samples[2*i+0]),
          y_R = hp1_2(samples[2*i+1]);
    samples[2*i+0] = short(y_L);
    samples[2*i+1] = short(y_R);
  }

  // 2nd second: cutoff at 2000 Hz
  HighPass1 hp2_1(2000,RATE);
  HighPass2 hp2_2(2000,RATE);
  for (unsigned i=0; i < RATE; ++i) {
    float y_L = hp2_1(samples[2*(RATE+i)+0]),
          y_R = hp2_2(samples[2*(RATE+i)+1]);
    samples[2*(RATE+i)+0] = short(y_L);
    samples[2*(RATE+i)+1] = short(y_R);
  }

  // 3rd second: cutoff at 8000 Hz
  HighPass1 hp3_1(8000,RATE);
  HighPass2 hp3_2(8000,RATE);
  for (unsigned i=0; i < RATE; ++i) {
    float y_L = hp3_1(samples[2*(2*RATE+i)+0]),
          y_R = hp3_2(samples[2*(2*RATE+i)+1]);
    samples[2*(2*RATE+i)+0] = short(y_L);
    samples[2*(2*RATE+i)+1] = short(y_R);
  }

  // 4th second: sweep of cutoff from 0 to 20000 Hz
  HighPass1 hp4_1(0,RATE);
  HighPass2 hp4_2(0,RATE);
  float factor = 20000.0f/RATE;
  for (unsigned i=0; i < RATE; ++i) {
    hp4_1.setFrequency(i*factor);
    hp4_2.setFrequency(i*factor);
    float y_L = hp4_1(samples[2*(3*RATE+i)+0]),
          y_R = hp4_2(samples[2*(3*RATE+i)+1]);
    samples[2*(3*RATE+i)+0] = short(y_L);
    samples[2*(3*RATE+i)+1] = short(y_R);
  }

  // write output file
  struct {
    char riff_chunk[4];
    unsigned chunk_size;
    char wave_fmt[4];
    char fmt_chunk[4];
    unsigned fmt_chunk_size;
    unsigned short audio_format;
    unsigned short number_of_channels;
    unsigned sampling_rate;
    unsigned bytes_per_second;
    unsigned short block_align;
    unsigned short bits_per_sample;
    char data_chunk[4];
    unsigned data_chunk_size;
  }
  header = { {'R','I','F','F'},
             36 + 4*count,
             {'W','A','V','E'},
             {'f','m','t',' '},
             16,1,2,RATE,4*RATE,4,16,
             {'d','a','t','a'},
             4*count
           };

  fstream out("HighPassTest.wav",ios_base::binary|ios_base::out);
  out.write(reinterpret_cast<char*>(&header),44);
  out.write(reinterpret_cast<char*>(samples),4*count);

  delete[] samples;
  return 0;
}

