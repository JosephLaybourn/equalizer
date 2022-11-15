// Filter.h
// -- general DSP filter interface
// cs246 9/18

#ifndef CS246_FILTER_H
#define CS246_FILTER_H


class Filter {
  public:
    virtual ~Filter(void) { }
    virtual float operator()(float x) = 0;
};


#endif

