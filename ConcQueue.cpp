#include "ConcQueue.h"

template <class T>
ConcQueue<T>::~ConcQueue()
{
	std::lock_guard<std::mutex> lock(_lock);
	_queue.clear();
}

template <class T>
T ConcQueue<T>::get()
{
	std::lock_guard<std::mutex> lock(_lock);
	T retval = _queue.front();
	_queue.pop_front();
	return retval;
}

template <class T>
void ConcQueue<T>::put(T t)
{
	std::lock_guard<std::mutex> lock(_lock);
	_queue.push_back(t);
}

template <class T>
void ConcQueue<T>::poison_self(T poison, size_t num_threads)
{
	std::lock_guard<std::mutex> lock(_lock);
	_queue.clear();
	for (size_t i = 0; i < num_threads; i++)
		_queue.push_back(poison);
}

/*
std::pair<Byte_Range, Buffer_ptr> ConcQueue::get_result(int curr_offset)
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
		Byte_Range br (NOT_FOUND, NOT_FOUND);
		Buffer_ptr bp = nullptr;
		return std::make_pair(br, bp);
	}
}

void ConcQueue::put_result(Byte_Range br, Buffer_ptr bp)
{
	std::lock_guard<std::mutex> lock(_lock);
	_queue.push_back(std::make_pair(br, bp));
}

Byte_Range ConcQueue::get_task()
{
	std::lock_guard<std::mutex> lock(_lock);
	for (auto it = _queue.begin(); it != _queue.end(); ++it) {
		if (it->second == nullptr) {
			Byte_Range br (it->first);
			_queue.erase(it);
			return br;
		}
	}
	for (auto& el : _queue) {
		if (el.second == nullptr) {
			br = el.first;
		}
	}
}

void ConcQueue::put_task(Byte_Range br)
{
}
*/

/*
void ConcQueue::poison_self(size_t num_threads)
{
	std::lock_guard<std::mutex> lock(_lock);
	_queue.clear();
	for (size_t i = 0; i < num_threads; ++i) {
		Byte_Range br (POISON, POISON);
		Buffer_ptr bp = nullptr;
		_queue.push_back(std::make_pair(br, bp));
	}
}
*/
