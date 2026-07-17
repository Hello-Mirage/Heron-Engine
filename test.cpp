
#include <iostream>
#include <tuple>

struct Comp { 
    int x; 
    long long padding; 
};

std::tuple<Comp&> get(Comp& c) { return std::tuple<Comp&>(c); }

int main() {
    Comp c{5, 0};
    auto [copy_c] = get(c);
    copy_c.x = 10;
    std::cout << "Original: " << c.x << std::endl;
    std::cout << "Copy: " << copy_c.x << std::endl;
    return 0;
}

