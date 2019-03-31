#include <deque>
#include <vector>
#include <mutex>
#include <functional>
#include <future>
#include <memory>

namespace snf {

template<typename T>
class sync_queue
{
private:
	std::deque<T>           m_queue;
	bool                    m_valid = true;
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

	bool empty()
	{
		std::lock_guard<std::mutex> guard(m_lock);
		return m_queue.empty();
	}

	size_t size()
	{
		std::lock_guard<std::mutex> guard(m_lock);
		return m_queue.size();
	}

	size_t waiting()
	{
		std::lock_guard<std::mutex> guard(m_lock);
		return m_waiting;
	}

	void put(const T &elem)
	{
		std::lock_guard<std::mutex> guard(m_lock);
		if (m_valid) {
			m_queue.push_back(elem);
			m_cv.notify_one();
		}
	}

	void put(T &&elem)
	{
		std::lock_guard<std::mutex> guard(m_lock);
		if (m_valid) {
			m_queue.emplace_back(std::move(elem));
			m_cv.notify_one();
		}
	}

	void get(T &elem)
	{
		std::unique_lock<std::mutex> lock(m_lock);
		m_waiting++;
		m_cv.wait(lock, [this] { return !(m_valid && m_queue.empty()); });
		m_waiting--;
		if (m_valid && !m_queue.empty()) {
			elem = std::move(m_queue.front());
			m_queue.pop_front();
		}
	}

	void invalidate()
	{
		std::lock_guard<std::mutex> guard(m_lock);
		m_valid = false;
		while (!m_queue.empty())
			m_queue.pop_back();
		m_cv.notify_all();
	}
};

class thread_pool
{
private:
	using fcn_type = std::function<void()>;

	std::atomic<bool>                           m_done { false };
	std::vector<std::unique_ptr<std::thread>>   m_threads;
	sync_queue<fcn_type>                        m_queue;

	void start(size_t n)
	{
		auto worker = [this] {
			while (!m_done) {
				fcn_type fcn;
				this->m_queue.get(fcn);
				(fcn)();
			}
		};

		for (size_t i = 0; i < n; ++i) {
			std::unique_ptr<std::thread> uniq_thrd{new std::thread{worker}};
			m_threads.emplace_back(std::move(uniq_thrd));
		}
	}

public:
	thread_pool()
	{
		size_t hc = std::thread::hardware_concurrency();
		hc--;
		if (hc <= 0)
			hc = 1;

		start(hc);
	}

	thread_pool(size_t n)
	{
		start(n);
	}

	~thread_pool() { stop(); }

	thread_pool(const thread_pool &) = delete;
	thread_pool(thread_pool &&) = delete;

	const thread_pool & operator= (const thread_pool &) = delete;
	thread_pool & operator= (thread_pool &&) = delete;

	void stop()
	{
		m_done = true;
		m_queue.invalidate();
		for (auto &t : m_threads) {
			if (t->joinable()) {
				t->join();
			}
		}
		m_threads.clear();
	}

	size_t thread_count() const { return m_threads.size(); }
	size_t request_count() { return m_queue.size(); }
	size_t threads_waiting() { return m_queue.waiting(); }

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
