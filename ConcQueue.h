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
		ConcQueue(ConcQueue<T>&& other)
		{
			_lock = std::move(other._lock);
			_queue = std::move(other._queue);
		}

		ConcQueue& operator=(const ConcQueue& other) = delete;
		ConcQueue& operator=(ConcQueue&& other)
		{
			_lock = std::move(other._lock);
			_queue = std::move(other._queue);
		}

		~ConcQueue()
		{
			std::lock_guard<std::mutex> lock(_lock);
			_queue.clear();
		}

		T get()
		{
			std::lock_guard<std::mutex> lock(_lock);
			T retval = _queue.front();
			_queue.pop_front();
			return retval;
		}

		void put(T t)
		{
			std::lock_guard<std::mutex> lock(_lock);
			_queue.push_back(t);
		}

		void poison_self(T poison, size_t num_threads)
		{
			std::lock_guard<std::mutex> lock(_lock);
			_queue.clear();
			for (size_t i = 0; i < num_threads; i++)
				_queue.push_back(poison);
		}

	private:
		std::deque<T> _queue;
		std::mutex _lock;
};

#endif
