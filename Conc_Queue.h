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
	int start;
	int stop;
};

using Buffer_ptr = std::shared_ptr<std::vector<char>>;

class Conc_Queue {
	public:

		// don't want to copy locks, undesired behavior
		Conc_Queue(const Conc_Queue& other) = delete;

		~Conc_Queue();

		std::pair<Byte_Range, Buffer_ptr> get(int curr_offset);

		void put(Byte_Range br, Buffer_ptr bp);

		void poison_self(size_t num_threads);

	private:
		std::vector<std::pair<Byte_Range, Buffer_ptr>>_queue;
		std::mutex _lock;
};

#endif
