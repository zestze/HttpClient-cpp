#ifndef __CONCQUEUE_H__
#define __CONCQUEUE_H__

#include <deque>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
//#include <optional>
#include <experimental/optional>

//@TODO: maybe build condition variable into the ConcQueue class?
// that way, get will block until the list has something in it.
//@TODO: can have variations on get()... with blocking_get() and timed_get()
//
//@TODO: put_and_wait(), get_and_notify() additional methods


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
