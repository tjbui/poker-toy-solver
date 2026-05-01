/* test_eval_pairs.cpp - test evaluator functionality (PAIRS / TRIPS) */

#include "evaluator.h"
#include "test_helpers.h"

int main() {
    std::cout << "\n========== START test_eval_pairs.cpp ==========\n\n";

    /* Hero:      AH AD
     * Villain:   KH QD
     * Community: 9S 7D 5H 2S 4C
     */
    print_test(
        "Pair beats high card",
        12, 25,
        11, 23,
        {46, 18, 3, 39, 28},
        HERO_WIN
    );

    /* Hero:      AH AD
     * Villain:   9H 8S
     * Community: 9S 8D 5H 2S 4C
     */
    print_test(
        "Two pair beats one pair",
        12, 25,
        7, 45,
        {46, 19, 3, 39, 28},
        VILLAIN_WIN
    );

    /* Hero:      AH AD
     * Villain:   KH KD
     * Community: AC 9S 9D 2S 4C
     */
    print_test(
        "Trips beats two pair",
        12, 25,
        11, 24,
        {38, 46, 20, 39, 28},
        HERO_WIN
    );

    /* Hero:      AH AD
     * Villain:   8S 9D
     * Community: AC 5H 6D 7C 2S
     */
    print_test(
        "Straight beats trips",
        12, 25,
        45, 20,
        {38, 3, 17, 31, 39},
        VILLAIN_WIN
    );

    /* Hero:      AH AD
     * Villain:   8S 9D
     * Community: KH 5H 6D 7C KS
     */
    print_test(
        "Straight beats two pair",
        12, 25,
        45, 20,
        {11, 3, 17, 31, 50},
        VILLAIN_WIN
    );

    /* Hero:      9S 5D
     * Villain:   KH QH
     * Community: 9H 5H 2H KS 4C
     */
    print_test(
        "Flush beats two pair",
        46, 16,
        11, 10,
        {7, 3, 0, 50, 28},
        VILLAIN_WIN
    );

    std::cout << "========== END test_eval_pairs.cpp ==========\n";
    return 0;
}