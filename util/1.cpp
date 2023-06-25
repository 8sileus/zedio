#include <iostream>
#include <vector>
#include "thread.hpp"

int main(){
    std::vector<std::thread> ts;
    for (int i = 0; i < 2;++i){
        ts.emplace_back(std::thread([]() {
            printf("%d: %d\n", 1, zed::util::getTid());
            std::this_thread::sleep_for(std::chrono::seconds(3));
            printf("%d: %d\n", 2, zed::util::getTid());
        }));
    }
    for(auto&t:ts){
        t.join();
    }
    return 0;
}