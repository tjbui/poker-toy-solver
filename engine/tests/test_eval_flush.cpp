/* test_eval_basic.cpp - test evaluator functionality (FLUSH) */

#include "evaluator.h"
#include "test_helpers.h"

int main() {
    std::cout << "\n========== START test_eval_flush.cpp ==========\n\n";

    /* Hero:      AH KH
     * Villain:   AS KD
     * Community: QH 9H 5H 2S 4C
     * Hero wins with Ace-high flush
     */
    print_test(
        "Hero Ace-high flush beats Villain high card",
        12, 11,
        51, 24,
        {10, 7, 3, 39, 28},
        HERO_WIN
    );

    /* Hero:      2H 3H
     * Villain:   AH KH
     * Community: QH 9H 5H 2S 4C
     * Villain wins with higher flush",
     */
    print_test(
        "Villain higher flush beats Hero lower flush",
        0, 1,
        12, 11,
        {10, 7, 3, 39, 28},
        VILLAIN_WIN
    );

    /* Hero:      2D 3C
     * Villain:   4S 5C
     * Community: AH KH QH 9H 5H
     * Tie because flush is entirely on board
     */
    print_test(
        "Board flush causes tie",
        13, 27,
        41, 29,
        {12, 11, 10, 7, 3},
        TIE
    );

    std::cout << "========== END test_eval_flush.cpp ==========\n";
    return 0;
}