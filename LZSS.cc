/************************************************/
/* LZSS.cc, (c) Rene Puchinger                  */
/************************************************/

#include "LZSS.h"
#include "FileIOStream.h"
#include <math.h>
#include <memory.h>

#define HASH(a, b, c, d) ((long) ((((d << 8) ^ (c << 5) + (b << 2)) + a) & (Searcher::HASH_SIZE - 1)))

/*****************************************************************************************/

LookAhead::LookAhead() {
	pos = 0;
	size = 0;
	memset((void*) data, 0, sizeof(unsigned char) * SIZE);
}

inline void LookAhead::add(const int c) {
	if (c != EOF) {
		data[pos] = (unsigned char) c;
		if (size < SIZE)
			size++;
	} else
		size--;
	pos++;
	pos %= SIZE;
}

inline long LookAhead::get_hash() {
	unsigned long a = (unsigned long) data[pos];
	unsigned long b = (unsigned long) data[(pos + 1) % SIZE];
	unsigned long c = (unsigned long) data[(pos + 2) % SIZE];
	unsigned long d = (unsigned long) data[(pos + 3) % SIZE];
	return HASH(a, b, c, d);
}

/*****************************************************************************************/

History::History() {
	pos = 0;
	memset((void*) data, 0, sizeof(unsigned char) * SIZE);
}

inline void History::add(const int c) {
	/* add a new character from the lookahead buffer to the current position and increment the position */
	data[pos] = (unsigned char) c;
	pos++;
	pos &= SIZE - 1;  /* because insert_pos is a power of 2, this is the same as modulo SIZE */
}

inline long History::get_hash() {
	unsigned long a = (unsigned long) data[pos];
	unsigned long b = (unsigned long) data[(pos + 1) & (SIZE - 1)];
	unsigned long c = (unsigned long) data[(pos + 2) & (SIZE - 1)];
	unsigned long d = (unsigned long) data[(pos + 3) & (SIZE - 1)];
	return HASH(a, b, c, d);
}

/*****************************************************************************************/

Searcher::Searcher(LookAhead *_look_ahead, History *_history) {
	look_ahead = _look_ahead;
	history = _history;
	for (long i = 0; i < History::SIZE; i++) {
		nodes[i].prev = -1;
		nodes[i].next = -1;
	}
	for (long i = 0; i < HASH_SIZE; i++) {
		hash_table[i].first = -1;
		hash_table[i].last = -1;
	}
}

inline long Searcher::cmp(const long search_pos) {
	long i = 0;
	while (history->at((search_pos + i) & (History::SIZE - 1)) == look_ahead->at((look_ahead->get_pos() + i) % LookAhead::SIZE))
		i++;
	return i;
}

void Searcher::find_match(long* position, long* length) {
	*length = LookAhead::MIN_LENGTH - 1;
	if (look_ahead->get_size() < LookAhead::MIN_LENGTH)
		return;
	long i = hash_table[look_ahead->get_hash()].last;
	if (i == -1)
		return;
	int cnt = 0;
	do {
		if (history->at((i + (*length)) & (History::SIZE - 1)) == look_ahead->at((look_ahead->get_pos() + (*length)) % LookAhead::SIZE)) {
			long l = cmp(i);
			if (l > *length) {
				*position = i;
				*length = l;
			}
		}
		i = nodes[i].prev;
	} while (i != -1 && cnt++ < SEARCH_DEPTH);
	if (*length > LookAhead::SIZE - 1)
		*length = LookAhead::SIZE - 1;
}

void Searcher::add() {
	long hash = look_ahead->get_hash();
	long last = hash_table[hash].last;
	if (last == -1) {
		hash_table[hash].first = history->get_pos();
	} else
		nodes[last].next = history->get_pos();
	nodes[history->get_pos()].prev = last;
	nodes[history->get_pos()].next = -1;
	hash_table[hash].last = history->get_pos();
}

void Searcher::remove() {
	long hash = history->get_hash();
	long remove_pos = hash_table[hash].first;
	if (remove_pos != -1) {
		long next = nodes[remove_pos].next;
		if (next != -1)
			nodes[next].prev = -1;
		else
			hash_table[hash].last = -1;
		nodes[remove_pos].next = -1;
		hash_table[hash].first = next;
	}
}

/*****************************************************************************************/

Encoder::Encoder() {
	look_ahead = new LookAhead;
	if (look_ahead == NULL)
		throw Exception(Exception::ERR_MEMORY);
	history = new History();
	if (history == NULL)
		throw Exception(Exception::ERR_MEMORY);
	searcher = new Searcher(look_ahead, history);
	if (searcher == NULL)
		throw Exception(Exception::ERR_MEMORY);
}

Encoder::~Encoder() {
	if (look_ahead)
		delete look_ahead;
	if (history)
		delete history;
	if (searcher)
		delete searcher;
}

void Encoder::encode(char* fn_in, char* fn_out) {
        FileInputStream* fin = new FileInputStream(fn_in);
        FileOutputStream* fout = new FileOutputStream(fn_out);
        BitOutputStream* bout = new BitOutputStream(fout);
	/* store the original stream size */
	bout->put_bits(fin->get_size(), 32);
	/* read first characters to the lookahead buffer */
	int c;
	while (look_ahead->get_size() < LookAhead::SIZE && (c = fin->get_char()) != EOF)
		look_ahead->add(c);
	look_ahead->reset_pos();
	while (look_ahead->get_size() > 0) {
		long position, length;
		searcher->find_match(&position, &length);
		if (length < LookAhead::MIN_LENGTH) {
			bout->put_bits(0, 1);
			bout->put_bits(look_ahead->first(), 8);
			searcher->remove();
			searcher->add();
			history->add(look_ahead->first());
			look_ahead->add(fin->get_char());
		} else {
			bout->put_bits(1, 1);
			bout->put_bits((unsigned long) position, History::NUM_BITS);
			bout->put_bits((unsigned long) (length - LookAhead::MIN_LENGTH), LookAhead::NUM_BITS);
			for (long k = length; k > 0; k--) {
				searcher->remove();
				searcher->add();
				history->add(look_ahead->first());
				look_ahead->add(fin->get_char());
			}
		}
	}
	bout->flush_bits();
        bout->flush();
        delete bout;
        delete fout;
        delete fin;

}

/*****************************************************************************************/

Decoder::Decoder() {
	look_ahead = new LookAhead;
	if (look_ahead == NULL)
		throw Exception(Exception::ERR_MEMORY);
	history = new History();
	if (history == NULL)
		throw Exception(Exception::ERR_MEMORY);
}

Decoder::~Decoder() {
	if (history)
		delete history;
	if (look_ahead)
		delete look_ahead;
}

void Decoder::decode(char* fn_in, char* fn_out) {
        FileInputStream* fin = new FileInputStream(fn_in);
        BitInputStream* bin = new BitInputStream(fin);
        FileOutputStream* fout = new FileOutputStream(fn_out);
	long fsize = bin->get_bits(32);
	while (fsize > 0) {
		if (bin->get_bits(1) == 0) {  /* single character */
			int c = bin->get_bits(8);
			fout->put_char(c);
			history->add(c);
			fsize--;
		} else {   /* (match, length) pair */
			long position = (long) bin->get_bits(History::NUM_BITS);
			long length = (long) bin->get_bits(LookAhead::NUM_BITS) + LookAhead::MIN_LENGTH;
			for (long i = 0; i < length; i++) {
				int c = history->at((position + i) & (History::SIZE - 1));
				look_ahead->add(c);
				fout->put_char(c);
				fsize--;
			}
			look_ahead->reset_pos();
			for (long i = 0; i < length; i++) {
				history->add(look_ahead->at(i));
			}
		}
	}
	fout->flush();
        delete fout;
        delete bin;
        delete fin;
}
