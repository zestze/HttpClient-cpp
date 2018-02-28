#ifndef __BYTERANGE_H__
#define __BYTERANGE_H__

// @NOTE: point of structure is to hold an inclusive range for
// each http Request to use.
//
// Thus, most methods are set to default since the structure is mostly for
// convencience.
//

struct ByteRange {
	ByteRange()
		:first{0}, last{0} { }
	ByteRange(int a, int b)
		:first{a}, last{b} { }

	ByteRange(const ByteRange& other) { first = other.first; last = other.last; }
	ByteRange(ByteRange&& other) = default;

	ByteRange& operator= (ByteRange&& other) = default;
	ByteRange& operator= (const ByteRange& rhs) = default;

	~ByteRange() = default;

	//@TODO: change from int to size_t
	int get_inclus_diff() const { return last - first + 1; }

	bool offset_matches(int offset) { return first == offset; }

	int first;
	int last;
};

#endif
