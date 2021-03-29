#include <deque>
#include <vector>
#include <mutex>
#include <functional>
#include <future>
#include <memory>

namespace snf {

/*
 * A synchronized queue. No copy or move operation allowed.
 * Elements are put in the queue using put() and fetched
 * using get() methods. waiting() could be used to get the
 * number of threads waiting on the get() method. The queue
 * could be made invalid (drained and all blocked thread
 * notified) using invalidate() method.
 */
template<typename T>
class sync_queue
{
private:
	std::deque<T>           m_queue;
	std::atomic<bool>       m_valid { true };
	size_t                  m_waiting = 0;
	std::mutex              m_lock;
	std::condition_variable m_cv;

public:
	sync_queue() {}
	virtual ~sync_queue() { invalidate(); }

	sync_queue(const sync_queue<T> &) = delete;
	sync_queue(sync_queue<T> &&) = delete;

	const sync_queue<T> & operator= (const sync_queue<T> &) = delete;
	sync_queue<T> & operator= (sync_queue<T> &&) = delete;

	// Is the queue empty?
	bool empty()
	{
		std::lock_guard<std::mutex> guard(m_lock);
		return m_queue.empty();
	}

	// Get the size of the queue.
	size_t size()
	{
		std::lock_guard<std::mutex> guard(m_lock);
		return m_queue.size();
	}

	// How many threads are waiting on the queue?
	size_t waiting()
	{
		std::lock_guard<std::mutex> guard(m_lock);
		return m_waiting;
	}

	// Put an element into the queue.
	void put(const T &elem)
	{
		std::lock_guard<std::mutex> guard(m_lock);
		if (m_valid) {
			m_queue.push_back(elem);
			m_cv.notify_one();
		}
	}

	// Put an element into the queue (using move).
	void put(T &&elem)
	{
		std::lock_guard<std::mutex> guard(m_lock);
		if (m_valid) {
			m_queue.emplace_back(std::move(elem));
			m_cv.notify_one();
		}
	}

	/*
	 * Get an element from the queue.
	 *
	 * @param [out] elem - element fetched from the queue.
	 *
	 * @return true if the element is fetched, false if the
	 *         element is not fetched.
	 */
	bool get(T &elem)
	{
		std::unique_lock<std::mutex> lock(m_lock);
		m_waiting++;
		m_cv.wait(lock, [this] {
			return !(this->m_valid && this->m_queue.empty());
		});
		m_waiting--;
		if (m_valid && !m_queue.empty()) {
			elem = std::move(m_queue.front());
			m_queue.pop_front();
			return true;
		}

		return false;
	}

	/*
	 * Invalidate the queue. The queue is drained and no operation
	 * can be done on the queue beyond this.
	 */
	void invalidate()
	{
		std::lock_guard<std::mutex> guard(m_lock);
		m_valid = false;
		while (!m_queue.empty())
			m_queue.pop_back();
		m_cv.notify_all();
	}
};

/*
 * A pool of threads. No copy or move operation allowed.
 * The user can specify the number of threads needed
 * via the constructor. Requests are submitted to the
 * pool using submit() methods.
 */
class thread_pool
{
private:
	using fcn_type = std::function<void()>;

	std::atomic<bool>                           m_done { false };
	std::vector<std::unique_ptr<std::thread>>   m_threads;
	sync_queue<fcn_type>                        m_queue;

	// Start the thread pool with 'n' threads.
	void start(size_t n)
	{
		auto worker = [this] {
			while (!this->m_done) {
				fcn_type fcn;
				if (this->m_queue.get(fcn))
					(fcn)();
			}
		};

		for (size_t i = 0; i < n; ++i) {
			std::unique_ptr<std::thread> uniq_thrd{DBG_NEW std::thread{worker}};
			m_threads.emplace_back(std::move(uniq_thrd));
		}
	}

public:
	/*
	 * Create and start the thread pool. Number of threads in
	 * the pool is determined by std::thread::hardware_concurrency() - 1.
	 */
	thread_pool()
	{
		size_t hc = std::thread::hardware_concurrency();
		hc--;
		if (hc <= 0)
			hc = 1;

		start(hc);
	}

	// Create and start the thread pool with 'n' threads.
	thread_pool(size_t n)
	{
		start(n);
	}

	// Stops and destroys the thread pool
	~thread_pool() { stop(); }

	thread_pool(const thread_pool &) = delete;
	thread_pool(thread_pool &&) = delete;

	const thread_pool & operator= (const thread_pool &) = delete;
	thread_pool & operator= (thread_pool &&) = delete;

	// how many threads are there in the thread pool?
	size_t thread_count() const { return m_threads.size(); }

	// how many requests are there in the thread pool?
	size_t request_count() { return m_queue.size(); }

	// how many threads are waiting for request in the thread pool?
	size_t threads_waiting() { return m_queue.waiting(); }

	// Stops the thread pool.
	void stop()
	{
		if (!m_done) {
			m_done = true;
			m_queue.invalidate();
			for (auto &t : m_threads) {
				if (t->joinable()) {
					t->join();
				}
			}
			m_threads.clear();
		}
	}

	// Submit request into the thread pool.
	template<typename Func, typename... Args>
	auto submit(Func && f, Args && ... args) -> std::future<decltype(f(args...))>
	{
		auto pkgd_task = std::make_shared<std::packaged_task<decltype(f(args...))()>>(
			std::bind(std::forward<Func>(f), std::forward<Args>(args)...)
		);
		fcn_type fcn = [pkgd_task] { (*pkgd_task)(); };
		m_queue.put(fcn);
		return pkgd_task->get_future();
	}

	// Submit request into the thread pool.
	template<typename Functor>
	auto submit(Functor && f) -> std::future<decltype(f())>
	{
		auto pkgd_task = std::make_shared<std::packaged_task<decltype(f())()>>(
			std::forward<Functor>(f)
		);
		fcn_type fcn = [pkgd_task] { (*pkgd_task)(); };
		m_queue.put(fcn);
		return pkgd_task->get_future();
	}
};

} // namespace snf
