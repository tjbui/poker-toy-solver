/* test_eval_basic.cpp - test basic evaluator functionality */

#include "evaluator.h"
#include "test_helpers.h"

int main() {
    std::cout << "\n========== START test_eval_basic.cpp ==========\n\n";

    /* Hero:      AH KD
     * Villain:   JH TS
     * Community: 9S AD 5H 2S 4C
     * Hero wins with pair of Aces
     */
    print_test(
        "Hero pair of Aces beats high card",
        12, 24,
        9, 47,
        {46, 25, 3, 39, 28},
        HERO_WIN
    );

    /* Hero:      2H 3D
     * Villain:   AS KC
     * Community: 9S 7D 5H 4C TD
     * Villain wins with Ace high
     */
    print_test(
        "Villain Ace high beats Hero Ten high",
        0, 14,
        51, 37,
        {46, 18, 3, 28, 21},
        VILLAIN_WIN
    );

    /* Hero:      AH KD
     * Villain:   AD KH
     * Community: 9S 7D 5H 2S 4C
     * Tie with same high cards
     */
    print_test(
        "Same best five cards tie",
        12, 24,
        25, 11,
        {46, 18, 3, 39, 28},
        TIE
    );

    std::cout << "========== END test_eval_basic.cpp ==========\n";
    return 0;
}