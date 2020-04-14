/************************************************/
/* Application.cc, (c) Rene Puchinger           */
/************************************************/

#include <iostream>
#include <ctime>
#include <cstdio>
#include "Application.h"

Application::Application(int argc, char** argv) {
    encoder = NULL;
    decoder = NULL;
    state = IDLE;

    std::cout << "\nLZSS v. 0.1\n(c) Rene Puchinger\n\n";

    if (argc == 4) {
        if (argv[1][0] == 'c') {
            encoder = new Encoder;
            state = ENCODE;
            fn_in = argv[2];
            fn_out = argv[3];
        } else if (argv[1][0] == 'd') {
            decoder = new Decoder;
            state = DECODE;
            fn_in = argv[2];
            fn_out = argv[3];
        }
    } else {
	std::cout << "Usage:   lzss [c|d] [file_in] [file_out]\n";
	std::cout << "Where:   c - compress\n";
	std::cout << "         d - decompress\n";
	std::cout << "Example: lzss c book.txt book.lzs\n";
	std::cout << "         lzss d book.lzs book.txt\n";
    }
}

Application::~Application() {
    if (encoder)
        delete encoder;
    if (decoder)
        delete decoder;
}

void Application::run() {
    clock_t start = clock();
    if (state == ENCODE) {
        std::cout << "Compressing ...\n";
        encoder->encode(fn_in, fn_out);
    } else if (state == DECODE) {
        std::cout << "Decompressing ...\n";
        decoder->decode(fn_in, fn_out);
    }
    if (state != IDLE) {
        std::cout << "Done! ";
        std::cout << "Time elapsed: " << (float) (clock() - start) / CLOCKS_PER_SEC << " seconds.\n";
    }
}
