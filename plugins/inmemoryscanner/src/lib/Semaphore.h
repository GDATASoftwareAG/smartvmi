#include <mutex>

namespace InMemoryScanner
{
    template <typename Mutex, typename CondVar> class Semaphore
    {
      public:
        explicit Semaphore(size_t count);

        Semaphore(const Semaphore&) = delete;

        Semaphore(Semaphore&&) = delete;

        Semaphore& operator=(const Semaphore&) = delete;

        Semaphore& operator=(Semaphore&&) = delete;

        void notify();

        void wait();

        bool try_wait();

      private:
        Mutex mMutex;
        CondVar mCv;
        size_t mCount;
    };

    template <typename Mutex, typename CondVar> Semaphore<Mutex, CondVar>::Semaphore(size_t count) : mCount{count} {}

    template <typename Mutex, typename CondVar> void Semaphore<Mutex, CondVar>::notify()
    {
        std::lock_guard<Mutex> lock{mMutex};
        ++mCount;
        mCv.notify_one();
    }

    template <typename Mutex, typename CondVar> void Semaphore<Mutex, CondVar>::wait()
    {
        std::unique_lock<Mutex> lock{mMutex};
        mCv.wait(lock, [&] { return mCount > 0; });
        --mCount;
    }

    template <typename Mutex, typename CondVar> bool Semaphore<Mutex, CondVar>::try_wait()
    {
        std::lock_guard<Mutex> lock{mMutex};

        if (mCount > 0)
        {
            --mCount;
            return true;
        }

        return false;
    }
}
