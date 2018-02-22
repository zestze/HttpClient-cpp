#include "Conc_Queue.h"

Conc_Queue::~Conc_Queue()
{
	std::lock_guard<std::mutex> lock(_lock);
	_queue.clear();
}

std::pair<Byte_Range, Buffer_ptr> Conc_Queue::get(int curr_offset)
{
	std::lock_guard<std::mutex> lock(_lock);
	auto it = _queue.begin();
	bool found = false;
	for (; it != _queue.end() && !found; ++it) {
		if (it->first.start == curr_offset)
			found = true;
	}
	if (found) {
		auto [br, bp] = *it;
		return std::make_pair(br, bp); //@TODO: needs to be tested
	}
	else {
		Byte_Range br;
		br.start = NOT_FOUND;
		br.stop = NOT_FOUND;
		Buffer_ptr bp = nullptr;
		return std::make_pair(br, bp);
	}
}

void Conc_Queue::put(Byte_Range br, Buffer_ptr bp)
{
	std::lock_guard<std::mutex> lock(_lock);
	_queue.push_back(std::make_pair(br, bp));
}

void Conc_Queue::poison_self(size_t num_threads)
{
	std::lock_guard<std::mutex> lock(_lock);
	_queue.clear();
	for (size_t i = 0; i < num_threads; ++i) {
		Byte_Range br;
		br.start = POISON;
		br.stop = POISON;
		Buffer_ptr bp = nullptr;
		_queue.push_back(std::make_pair(br, bp));
	}
}

