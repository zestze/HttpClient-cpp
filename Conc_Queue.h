#ifndef __CONC_QUEUE_H__
#define __CONC_QUEUE_H__

#include <deque>
#include <vector>
#include <memory>
#include <mutex>

#ifndef NOT_FOUND
#define NOT_FOUND -1
#endif

#ifndef POISON_
#define POISON -2
#endif

struct Byte_Range {
	Byte_Range()
		:start{0}, stop{0} { }
	Byte_Range(int a, int b)
		:start{a}, stop{b} { }
	Byte_Range(const Byte_Range& other) { start = other.start; stop = other.stop; }
	Byte_Range& operator= (const Byte_Range& rhs)
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

using Buffer_ptr = std::shared_ptr<std::vector<char>>;

template <typename T>
class Conc_Queue {
	public:
		Conc_Queue() = default;

		// don't want to copy locks, undesired behavior
		Conc_Queue(const Conc_Queue& other) = delete;

		~Conc_Queue();

		// if an element has a ByteRange that matches 
		std::pair<Byte_Range, Buffer_ptr> get_result(int curr_offset);

		void put_result(Byte_Range br, Buffer_ptr bp);

		Byte_Range get_task();

		void poison_self(size_t num_threads);

	private:
		std::vector<std::pair<Byte_Range, Buffer_ptr>>_queue;
		std::mutex _lock;
};

#endif
