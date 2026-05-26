/* engine_cuda_demo.cu - standalone CUDA poker equity simulation demo */
/* nvcc -O2 -std=c++17 engine_cuda_demo.cu -o engine_cuda_demo        */
/* ./engine_cuda_demo Ah Ad Kc Qc 100000                              */

#include <cuda_runtime.h>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

using Card = uint8_t;

/*
 * Checks CUDA API calls and exits if any CUDA error occurs.
 */
#define CUDA_CHECK(call)                                                        \
    do {                                                                        \
        cudaError_t err = (call);                                                \
        if (err != cudaSuccess) {                                                \
            std::cerr << "CUDA error at " << __FILE__ << ":" << __LINE__         \
                      << " -> " << cudaGetErrorString(err) << std::endl;         \
            std::exit(1);                                                        \
        }                                                                       \
    } while (0)

/*
 * Poker hand categories. Larger category means stronger hand.
 */
enum hand_rank_t {
    HIGH_CARD = 0,
    ONE_PAIR = 1,
    TWO_PAIR = 2,
    THREE_OF_A_KIND = 3,
    STRAIGHT = 4,
    FLUSH = 5,
    FULL_HOUSE = 6,
    FOUR_OF_A_KIND = 7,
    STRAIGHT_FLUSH = 8
};

/*
 * Parses rank character into internal rank value.
 * 2 = 0, 3 = 1, ..., A = 12.
 */
int parse_rank(char rank_char) {
    if (rank_char >= '2' && rank_char <= '9') {
        return rank_char - '2';
    } else if (rank_char == 'T' || rank_char == 't') {
        return 8;
    } else if (rank_char == 'J' || rank_char == 'j') {
        return 9;
    } else if (rank_char == 'Q' || rank_char == 'q') {
        return 10;
    } else if (rank_char == 'K' || rank_char == 'k') {
        return 11;
    } else if (rank_char == 'A' || rank_char == 'a') {
        return 12;
    }

    throw std::invalid_argument("Invalid rank.");
} /* parse_rank() */

/*
 * Parses suit character into internal suit value.
 * clubs = 0, diamonds = 1, hearts = 2, spades = 3.
 */
int parse_suit(char suit_char) {
    if (suit_char == 'c' || suit_char == 'C') {
        return 0;
    } else if (suit_char == 'd' || suit_char == 'D') {
        return 1;
    } else if (suit_char == 'h' || suit_char == 'H') {
        return 2;
    } else if (suit_char == 's' || suit_char == 'S') {
        return 3;
    }

    throw std::invalid_argument("Invalid suit.");
} /* parse_suit() */

/*
 * Parses card string into compact Card representation.
 * Card representation is suit * 13 + rank.
 */
Card parse_card(const std::string& card_str) {
    if (card_str.size() != 2) {
        throw std::invalid_argument("Card must look like Ah, Kc, Td, 7s, etc.");
    }

    int rank = parse_rank(card_str[0]);
    int suit = parse_suit(card_str[1]);

    return static_cast<Card>(suit * 13 + rank);
} /* parse_card() */

/*
 * Converts internal Card representation back into readable string.
 */
std::string card_to_string(Card card) {
    static const char* ranks = "23456789TJQKA";
    static const char* suits = "cdhs";

    int rank = card % 13;
    int suit = card / 13;

    std::string card_str;
    card_str.push_back(ranks[rank]);
    card_str.push_back(suits[suit]);

    return card_str;
} /* card_to_string() */

/*
 * Validates that all four hole cards are distinct.
 */
void validate_distinct_cards(Card hero1, Card hero2, Card villain1, Card villain2) {
    if (hero1 == hero2 || hero1 == villain1 || hero1 == villain2 ||
        hero2 == villain1 || hero2 == villain2 || villain1 == villain2) {
        throw std::invalid_argument("Input hole cards must be distinct.");
    }
} /* validate_distinct_cards() */

/*
 * Returns card rank on device.
 */
__device__ int card_rank(Card card) {
    return card % 13;
} /* card_rank() */

/*
 * Returns card suit on device.
 */
__device__ int card_suit(Card card) {
    return card / 13;
} /* card_suit() */

/*
 * Simple device-side xorshift RNG.
 * This is not cryptographic. It is only used for Monte Carlo sampling.
 */
__device__ uint32_t xorshift32(uint32_t& state) {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;

    return state;
} /* xorshift32() */

/*
 * Compresses score into a 64-bit int.
 *
 * Format:
 * [category][rank1][rank2][rank3][rank4][rank5]
 *
 * Larger score means stronger poker hand.
 */
__device__ uint64_t make_score(int category, int rank1, int rank2,
                               int rank3, int rank4, int rank5) {
    uint64_t score = static_cast<uint64_t>(category);
    int ranks[5] = {rank1, rank2, rank3, rank4, rank5};

    for (int i = 0; i < 5; i++) {
        int packed_rank = 0;

        if (ranks[i] >= 0) {
            packed_rank = ranks[i] + 1;
        }

        score = score * 14ULL + static_cast<uint64_t>(packed_rank);
    }

    return score;
} /* make_score() */

/*
 * Finds the highest rank of a straight from a rank bit mask.
 */
__device__ int find_straight_high_from_mask(int rank_mask) {
    for (int high = 12; high >= 4; high--) {
        int needed = 0;

        for (int offset = 0; offset < 5; offset++) {
            needed |= 1 << (high - offset);
        }

        if ((rank_mask & needed) == needed) {
            return high;
        }
    }

    int wheel_mask = (1 << 12) | (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3);

    if ((rank_mask & wheel_mask) == wheel_mask) {
        return 3;
    }

    return -1;
} /* find_straight_high_from_mask() */

/*
 * Evaluates a 7-card poker hand on the GPU.
 *
 * This function mirrors the CPU evaluator idea, but avoids vectors and std::array
 * because this runs inside CUDA device code.
 */
__device__ uint64_t evaluate_7_cards(const Card cards[7]) {
    int rank_counts[13] = {0};
    int suit_counts[4] = {0};
    int rank_mask = 0;
    int suit_rank_masks[4] = {0, 0, 0, 0};

    for (int i = 0; i < 7; i++) {
        int rank = card_rank(cards[i]);
        int suit = card_suit(cards[i]);

        rank_counts[rank]++;
        suit_counts[suit]++;
        rank_mask |= 1 << rank;
        suit_rank_masks[suit] |= 1 << rank;
    }

    /* straight flush */
    for (int suit = 0; suit < 4; suit++) {
        if (suit_counts[suit] >= 5) {
            int high = find_straight_high_from_mask(suit_rank_masks[suit]);

            if (high != -1) {
                return make_score(STRAIGHT_FLUSH, high, -1, -1, -1, -1);
            }
        }
    }

    /* four of a kind */
    for (int rank = 12; rank >= 0; rank--) {
        if (rank_counts[rank] == 4) {
            int kicker = -1;

            for (int k = 12; k >= 0; k--) {
                if (k != rank && rank_counts[k] > 0) {
                    kicker = k;
                    break;
                }
            }

            return make_score(FOUR_OF_A_KIND, rank, kicker, -1, -1, -1);
        }
    }

    /* full house */
    int trip = -1;
    int pair = -1;

    for (int rank = 12; rank >= 0; rank--) {
        if (rank_counts[rank] >= 3 && trip == -1) {
            trip = rank;
        }
    }

    if (trip != -1) {
        for (int rank = 12; rank >= 0; rank--) {
            if (rank != trip && rank_counts[rank] >= 2) {
                pair = rank;
                break;
            }
        }
    }

    if (trip != -1 && pair != -1) {
        return make_score(FULL_HOUSE, trip, pair, -1, -1, -1);
    }

    /* flush */
    for (int suit = 0; suit < 4; suit++) {
        if (suit_counts[suit] >= 5) {
            int flush_ranks[5] = {-1, -1, -1, -1, -1};
            int count = 0;

            for (int rank = 12; rank >= 0 && count < 5; rank--) {
                if (suit_rank_masks[suit] & (1 << rank)) {
                    flush_ranks[count] = rank;
                    count++;
                }
            }

            return make_score(FLUSH, flush_ranks[0], flush_ranks[1],
                              flush_ranks[2], flush_ranks[3], flush_ranks[4]);
        }
    }

    /* straight */
    int straight_high = find_straight_high_from_mask(rank_mask);

    if (straight_high != -1) {
        return make_score(STRAIGHT, straight_high, -1, -1, -1, -1);
    }

    /* three of a kind */
    for (int rank = 12; rank >= 0; rank--) {
        if (rank_counts[rank] == 3) {
            int kickers[2] = {-1, -1};
            int count = 0;

            for (int k = 12; k >= 0 && count < 2; k--) {
                if (k != rank && rank_counts[k] > 0) {
                    kickers[count] = k;
                    count++;
                }
            }

            return make_score(THREE_OF_A_KIND, rank, kickers[0], kickers[1],
                              -1, -1);
        }
    }

    /* two pair */
    int high_pair = -1;
    int low_pair = -1;

    for (int rank = 12; rank >= 0; rank--) {
        if (rank_counts[rank] >= 2) {
            if (high_pair == -1) {
                high_pair = rank;
            } else {
                low_pair = rank;
                break;
            }
        }
    }

    if (high_pair != -1 && low_pair != -1) {
        int kicker = -1;

        for (int k = 12; k >= 0; k--) {
            if (k != high_pair && k != low_pair && rank_counts[k] > 0) {
                kicker = k;
                break;
            }
        }

        return make_score(TWO_PAIR, high_pair, low_pair, kicker, -1, -1);
    }

    /* one pair */
    for (int rank = 12; rank >= 0; rank--) {
        if (rank_counts[rank] == 2) {
            int kickers[3] = {-1, -1, -1};
            int count = 0;

            for (int k = 12; k >= 0 && count < 3; k--) {
                if (k != rank && rank_counts[k] > 0) {
                    kickers[count] = k;
                    count++;
                }
            }

            return make_score(ONE_PAIR, rank, kickers[0], kickers[1],
                              kickers[2], -1);
        }
    }

    /* high card */
    int high_cards[5] = {-1, -1, -1, -1, -1};
    int count = 0;

    for (int rank = 12; rank >= 0 && count < 5; rank--) {
        if (rank_counts[rank] > 0) {
            high_cards[count] = rank;
            count++;
        }
    }

    return make_score(HIGH_CARD, high_cards[0], high_cards[1],
                      high_cards[2], high_cards[3], high_cards[4]);
} /* evaluate_7_cards() */

/*
 * CUDA kernel that runs independent Monte Carlo simulations in parallel.
 *
 * Each CUDA thread handles a subset of total trials using a grid-stride loop.
 */
__global__ void simulate_equity_kernel(Card hero1, Card hero2,
                                       Card villain1, Card villain2,
                                       int simulations, uint64_t seed,
                                       unsigned long long* hero_wins,
                                       unsigned long long* villain_wins,
                                       unsigned long long* ties) {
    int thread_id = blockIdx.x * blockDim.x + threadIdx.x;
    int stride = blockDim.x * gridDim.x;

    unsigned long long local_hero_wins = 0;
    unsigned long long local_villain_wins = 0;
    unsigned long long local_ties = 0;

    uint32_t rng_state =
        static_cast<uint32_t>(seed ^ (thread_id * 747796405u + 2891336453u));

    for (int sim = thread_id; sim < simulations; sim += stride) {
        uint64_t used_cards = 0;

        used_cards |= 1ULL << hero1;
        used_cards |= 1ULL << hero2;
        used_cards |= 1ULL << villain1;
        used_cards |= 1ULL << villain2;

        Card board[5];

        for (int i = 0; i < 5; i++) {
            Card card;

            do {
                card = static_cast<Card>(xorshift32(rng_state) % 52);
            } while (used_cards & (1ULL << card));

            used_cards |= 1ULL << card;
            board[i] = card;
        }

        Card hero_cards[7] = {
            hero1, hero2, board[0], board[1], board[2], board[3], board[4]
        };

        Card villain_cards[7] = {
            villain1, villain2, board[0], board[1], board[2], board[3], board[4]
        };

        uint64_t hero_score = evaluate_7_cards(hero_cards);
        uint64_t villain_score = evaluate_7_cards(villain_cards);

        if (hero_score > villain_score) {
            local_hero_wins++;
        } else if (villain_score > hero_score) {
            local_villain_wins++;
        } else {
            local_ties++;
        }
    }

    atomicAdd(hero_wins, local_hero_wins);
    atomicAdd(villain_wins, local_villain_wins);
    atomicAdd(ties, local_ties);
} /* simulate_equity_kernel() */

/*
 * Runs CUDA Monte Carlo simulation and prints benchmark results.
 */
int main(int argc, char** argv) {
    std::string hero1_str = "Ah";
    std::string hero2_str = "Ad";
    std::string villain1_str = "Kc";
    std::string villain2_str = "Qc";
    int simulations = 1000000;

    if (argc == 6) {
        hero1_str = argv[1];
        hero2_str = argv[2];
        villain1_str = argv[3];
        villain2_str = argv[4];
        simulations = std::stoi(argv[5]);
    } else {
        std::cout << "Usage:\n";
        std::cout << "  " << argv[0]
                  << " <hero1> <hero2> <villain1> <villain2> <simulations>\n\n";
        std::cout << "No args provided. Running default:\n";
        std::cout << "  " << argv[0] << " Ah Ad Kc Qc 1000000\n\n";
    }

    /*
     * Forces CUDA runtime initialization before timing.
     * This keeps first-time CUDA context creation out of the benchmark.
     */
    CUDA_CHECK(cudaFree(0));

    auto total_start = std::chrono::high_resolution_clock::now();

    Card hero1 = parse_card(hero1_str);
    Card hero2 = parse_card(hero2_str);
    Card villain1 = parse_card(villain1_str);
    Card villain2 = parse_card(villain2_str);

    validate_distinct_cards(hero1, hero2, villain1, villain2);

    unsigned long long* d_hero_wins = nullptr;
    unsigned long long* d_villain_wins = nullptr;
    unsigned long long* d_ties = nullptr;

    auto setup_start = std::chrono::high_resolution_clock::now();

    CUDA_CHECK(cudaMalloc(&d_hero_wins, sizeof(unsigned long long)));
    CUDA_CHECK(cudaMalloc(&d_villain_wins, sizeof(unsigned long long)));
    CUDA_CHECK(cudaMalloc(&d_ties, sizeof(unsigned long long)));

    CUDA_CHECK(cudaMemset(d_hero_wins, 0, sizeof(unsigned long long)));
    CUDA_CHECK(cudaMemset(d_villain_wins, 0, sizeof(unsigned long long)));
    CUDA_CHECK(cudaMemset(d_ties, 0, sizeof(unsigned long long)));

    auto setup_end = std::chrono::high_resolution_clock::now();

    int threads_per_block = 256;
    int blocks = 256;
    uint64_t seed = 123456789ULL;

    cudaEvent_t kernel_start;
    cudaEvent_t kernel_stop;

    CUDA_CHECK(cudaEventCreate(&kernel_start));
    CUDA_CHECK(cudaEventCreate(&kernel_stop));

    CUDA_CHECK(cudaEventRecord(kernel_start));

    simulate_equity_kernel<<<blocks, threads_per_block>>>(
        hero1, hero2, villain1, villain2, simulations, seed,
        d_hero_wins, d_villain_wins, d_ties
    );

    CUDA_CHECK(cudaGetLastError());

    CUDA_CHECK(cudaEventRecord(kernel_stop));
    CUDA_CHECK(cudaEventSynchronize(kernel_stop));

    float kernel_ms = 0.0f;
    CUDA_CHECK(cudaEventElapsedTime(&kernel_ms, kernel_start, kernel_stop));

    auto copy_start = std::chrono::high_resolution_clock::now();

    unsigned long long hero_wins = 0;
    unsigned long long villain_wins = 0;
    unsigned long long ties = 0;

    CUDA_CHECK(cudaMemcpy(&hero_wins, d_hero_wins,
                          sizeof(unsigned long long), cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaMemcpy(&villain_wins, d_villain_wins,
                          sizeof(unsigned long long), cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaMemcpy(&ties, d_ties,
                          sizeof(unsigned long long), cudaMemcpyDeviceToHost));

    auto copy_end = std::chrono::high_resolution_clock::now();

    CUDA_CHECK(cudaEventDestroy(kernel_start));
    CUDA_CHECK(cudaEventDestroy(kernel_stop));

    CUDA_CHECK(cudaFree(d_hero_wins));
    CUDA_CHECK(cudaFree(d_villain_wins));
    CUDA_CHECK(cudaFree(d_ties));

    auto total_end = std::chrono::high_resolution_clock::now();

    double setup_ms =
        std::chrono::duration<double, std::milli>(setup_end - setup_start).count();
    double copy_ms =
        std::chrono::duration<double, std::milli>(copy_end - copy_start).count();
    double total_ms =
        std::chrono::duration<double, std::milli>(total_end - total_start).count();

    double kernel_seconds = kernel_ms / 1000.0;
    double total_seconds = total_ms / 1000.0;

    double kernel_simulations_per_second =
        static_cast<double>(simulations) / kernel_seconds;
    double total_simulations_per_second =
        static_cast<double>(simulations) / total_seconds;

    double hero_equity =
        (static_cast<double>(hero_wins) + 0.5 * static_cast<double>(ties)) /
        static_cast<double>(simulations);

    double villain_equity =
        (static_cast<double>(villain_wins) + 0.5 * static_cast<double>(ties)) /
        static_cast<double>(simulations);

    std::cout << std::fixed << std::setprecision(4);

    std::cout << "Hero:    " << card_to_string(hero1) << " "
              << card_to_string(hero2) << "\n";
    std::cout << "Villain: " << card_to_string(villain1) << " "
              << card_to_string(villain2) << "\n";
    std::cout << "Simulations: " << simulations << "\n";
    std::cout << "Blocks:      " << blocks << "\n";
    std::cout << "Threads per block: " << threads_per_block << "\n";
    std::cout << "Total CUDA threads launched: "
              << blocks * threads_per_block << "\n\n";

    std::cout << "Hero wins:    " << hero_wins << "\n";
    std::cout << "Villain wins: " << villain_wins << "\n";
    std::cout << "Ties:         " << ties << "\n\n";

    std::cout << "Hero equity:    " << hero_equity * 100.0 << "%\n";
    std::cout << "Villain equity: " << villain_equity * 100.0 << "%\n\n";

    std::cout << "Benchmark:\n";
    std::cout << "  GPU setup time:        " << setup_ms << " ms\n";
    std::cout << "  GPU kernel time only:  " << kernel_ms << " ms\n";
    std::cout << "  GPU copy-back time:    " << copy_ms << " ms\n";
    std::cout << "  Total wall time:       " << total_ms << " ms\n\n";

    std::cout << "Throughput:\n";
    std::cout << "  Kernel-only:           "
              << kernel_simulations_per_second << " simulations/sec\n";
    std::cout << "  Total wall-time:       "
              << total_simulations_per_second << " simulations/sec\n";

    return 0;
} /* main() */