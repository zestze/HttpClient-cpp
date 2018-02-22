#ifndef __CONCQUEUE_H__
#define __CONCQUEUE_H__

#include <deque>
#include <vector>
#include <memory>
#include <mutex>


template <class T>
class ConcQueue {
	public:
		ConcQueue() = default;

		// don't want to copy locks, undesired behavior
		ConcQueue(const ConcQueue<T>& other) = delete;

		~ConcQueue();

		T get();

		void put(T t);

		void poison_self(T poison, size_t num_threads);

	private:
		std::deque<T> _queue;
		std::mutex _lock;
};

#endif
