/************************************************/
/* LZSS.h, (c) Rene Puchinger                   */
/*                                              */
/* classes for compression and decompression    */
/************************************************/

#ifndef LZSS_H
#define LZSS_H

#include "IOStream.h"
#include "BitIOStream.h"
#include "Exception.h"

/* the lookahead buffer */
class LookAhead {
public:
	static const long NUM_BITS = 8;
	static const long MIN_LENGTH = 4;   /* minimal length for a string to be encoded as (offset, match) pair */
	static const long SIZE = (1 << NUM_BITS) + MIN_LENGTH;
private:
	unsigned char data[SIZE];
	long pos;                                       /* insertion position - is always present as modulo SIZE */
	long size;                                      /* true size, can be less than SIZE */
public:
	LookAhead();
	void add(const int c);                          /* add a new character at the insertion position */
	unsigned char at(const long i) { return data[i]; }       /* return the character at position i */
	long get_size() { return size; }
	long get_pos() { return pos; }
	unsigned char first() { return data[pos]; }
	void reset_pos() { pos = 0; }
	long get_hash();                                /* compute the hash of first 4 chars */
};

/* the history of strings for compression */
class History {
public:
	static const long NUM_BITS = 16;                /* number of encoding bits */
	static const long SIZE = (1 << NUM_BITS);       /* history size */
private:
	unsigned char data[SIZE];
	long pos;                                       /* insertion position - is always present as modulo SIZE */
public:
	History();
	void add(const int c);                          /* add a new character to the current position */
	unsigned char at(const long i) { return data[i]; }       /* return the character at position i */
	long get_pos() { return pos; }
	long get_hash();                                /* compute the same hash as in LookAhead */
};

/* a class for fast searching */
class Searcher {
public:
	static const long HASH_SIZE = History::SIZE;  /* hash table size */
	static const int SEARCH_DEPTH = 512;          /* number of nodes to search */
private:
	struct {
		long prev;                            /* previous node with the same hash */
		long next;
	} nodes[History::SIZE];
	struct {
		long first;                           /* first node with a corresponding hash */
		long last;                            /* last node with a corresponding hash */
	} hash_table[HASH_SIZE];
	LookAhead* look_ahead;
	History* history;
	long cmp(const long search_pos);
public:
	Searcher(LookAhead* _look_ahead, History* _history);
	/* find the longest match and store its position and length; if no match of length
	   at least MIN_LENGTH was found, set length to 0 */
	void find_match(long* position, long* length);
	/* add the string from the lookahead buffer to the hash table */
	void add();
	/* remove the last string from the hash table */
	void remove();
};

/* class for compression */
class Encoder {
private:
	History* history;
	LookAhead* look_ahead;
	Searcher* searcher;
public:
	Encoder();
	~Encoder();
	void encode(char* fn_in, char* fn_out);
};

/* class for decompression */
class Decoder {
private:
	History* history;
	LookAhead* look_ahead;
public:
	Decoder();
	~Decoder();
	void decode(char* fn_in, char* fn_out);
};

#endif