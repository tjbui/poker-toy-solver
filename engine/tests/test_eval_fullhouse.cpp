/* test_eval_fullhouse.cpp - test evaluator functionality (FULL HOUSE) */

#include "evaluator.h"
#include "test_helpers.h"

int main() {
    std::cout << "\n========== START test_eval_fullhouse.cpp ==========\n\n";

    /* Hero:      AH AD
     * Villain:   KH KD
     * Community: AC KS 2H 2D 4C
     */
    print_test(
        "Hero wins with Aces full of 2s vs Kings full of 2s",
        12, 25,
        11, 24,
        {38, 50, 0, 13, 28},
        HERO_WIN
    );

    /* Hero:      KH KD
     * Villain:   AH 7D
     * Community: AC KC AD 7S 4C
     */
    print_test(
        "Kings full of Aces vs Aces full of 7s",
        11, 24,
        12, 18,
        {38, 37, 25, 44, 28},
        VILLAIN_WIN
    );

    /* Hero:      AH AD
     * Villain:   8S 9D
     * Community: AC 5H 6D 7C 5S
     * Hero wins with Aces full of 2s vs Villain 5-9 straight
     */
    print_test(
        "Full house beats straight",
        12, 25,
        45, 20,
        {38, 3, 17, 31, 42},
        HERO_WIN
    );

    /* Hero:      2H 2D
     * Villain:   AH KH
     * Community: 2C QS QH 5H 3H
     * Hero wins with 2s full of 3s vs Villain Ace-high flush
     */
    print_test(
        "Full house beats flush",
        0, 13,
        12, 11,
        {26, 49, 10, 3, 1},
        HERO_WIN
    );

    std::cout << "========== END test_eval_fullhouse.cpp ==========\n";
    return 0;
}