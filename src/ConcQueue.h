// @NOTE:
// Currently this file is more or less obsolete, and not in use
// A relic of a previous implementation that tried synchronizing
// access to the file writer.
//
// @NOTE:
// This class simulates a Queue with Concurrent access.
//

#ifndef __CONCQUEUE_H__
#define __CONCQUEUE_H__

#include <deque>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
//#include <optional>
#include <experimental/optional>


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
			std::unique_lock<std::mutex> lock(_lock);
			_queue.clear();
			lock.unlock(); //@NOTE: bc worried destructor is
			// getting called before leaving execcution block
		}

		std::experimental::optional<T> get()
		{
			std::lock_guard<std::mutex> lock(_lock);
			if (_size == 0)
				return std::experimental::nullopt;
			T retval = std::move(_queue.front());
			_queue.pop_front();
			_size--;
			return retval;
		}

		void put_front(T t)
		{
			std::lock_guard<std::mutex> lock(_lock);
			_queue.push_front(t);
			_size++;
		}

		void put(T t)
		{
			std::lock_guard<std::mutex> lock(_lock);
			_queue.push_back(t);
			_size++;
		}

		void poison_self(T poison, size_t num_threads)
		{
			std::lock_guard<std::mutex> lock(_lock);
			_queue.clear();
			for (size_t i = 0; i < num_threads; i++)
				_queue.push_back(poison);
			_size = num_threads;
		}

		bool is_empty() { return _size == 0; }

	private:
		std::deque<T> _queue;
		std::mutex _lock;
		std::atomic<int> _size;
};

#endif
