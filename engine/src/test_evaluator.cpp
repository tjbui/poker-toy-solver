#include "evaluator.h"
#include <iostream>

int main() {
    std::cout << "hello\n";
    std::array<Card, 5> community = {7, 8, 9, 10, 11};
    evaluate_hands(1, 2, 3, 4, community);
}