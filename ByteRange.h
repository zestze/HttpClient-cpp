#ifndef __BYTERANGE_H__
#define __BYTERANGE_H__

struct ByteRange {
	ByteRange()
		:start{0}, stop{0} { }
	ByteRange(int a, int b)
		:start{a}, stop{b} { }
	ByteRange(const ByteRange& other) { start = other.start; stop = other.stop; }
	ByteRange& operator= (const ByteRange& rhs)
	{
		if (this == &rhs)
			return *this;
		start = rhs.start;
		stop = rhs.stop;
		return *this;
	}

	int start;
	int stop;
};

#endif
