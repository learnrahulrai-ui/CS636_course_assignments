#include <iostream>

class GlobalState {
public:
    int shared_value;

    GlobalState() {
        shared_value = 100; 
    }
};

GlobalState my_global_app_state;

int main() {
    std::cout << "Application started. Value: " << my_global_app_state.shared_value << std::endl;
    return 0;
}