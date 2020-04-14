/************************************************/
/* Application.h, (c) Rene Puchinger            */
/*                                              */
/* sample wavelet image compression program     */
/************************************************/

#ifndef APPLICATION_H
#define APPLICATION_H

#include "LZSS.h"

class Application {
    enum { ENCODE, DECODE, IDLE } state;
    Encoder* encoder;
    Decoder* decoder;
    char* fn_in;
    char* fn_out;
public:
    Application(int argc, char** argv);
    ~Application();
    void run();
};

#endif
