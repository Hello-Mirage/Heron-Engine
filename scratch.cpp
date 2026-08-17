#include <iostream>
#include <tuple>

struct Comp { int x; };

int main() {
    Comp c{5};
    std::tuple<Comp&> tup(c);
    
    auto [copy_or_ref] = tup;
    copy_or_ref.x = 10;
    
    std::cout << "c.x is " << c.x << std::endl;
    return 0;
}
