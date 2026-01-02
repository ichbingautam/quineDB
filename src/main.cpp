#include <iostream>
#include <thread>
#include <vector>

void worker(int id) {
    std::cout << "Worker " << id << " started on thread " << std::this_thread::get_id() << std::endl;
}

int main() {
    unsigned int n_threads = std::thread::hardware_concurrency();
    std::cout << "ZephyraDB Server starting on " << n_threads << " cores." << std::endl;

    std::vector<std::thread> threads;
    for (unsigned int i = 0; i < n_threads; ++i) {
        threads.emplace_back(worker, i);
    }

    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    return 0;
}
