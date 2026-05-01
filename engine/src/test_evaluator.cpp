/* test_evaluator.cpp - test module for evaluator */

#include "evaluator.h"
#include <iostream>

void print_result(result_t res) {
    if (res == HERO_WIN) std::cout << "Hero win\n";
    if (res == VILLAIN_WIN) std::cout << "Villain win\n";
    else std::cout << "Tie";
}

int main() {
    std::array<Card, 5> community = {7, 8, 9, 10, 11};


    print_result(evaluate_hands(1, 2, 3, 4, community));
}