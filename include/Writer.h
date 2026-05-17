#ifndef WRITER_H
#define WRITER_H

#include <string>
#include "Allocator.h"   // for AllocationResult

/**
 * @brief Static utility class responsible for writing the final allocation result to an output file.
 * * @details Formats the output strictly according to the project specifications (Figures 11 & 12).
 * The output format follows this structure:
 *
 * @code
 * # Total number of webs followed by the listing of the program points of each one
 * # program points in each web are sorted in ascending order
 * webs: <N>
 * web0: <points...>
 * web1: <points...>
 * ...
 * # Total number of registers used, followed by assignment to webs
 * registers: <R>
 * r0: web2
 * r0: web5
 * r1: web0
 * ...
 * M: web3       <- spilled web
 * @endcode
 */
class Writer {
public:
    /**
     * @brief Writes the computed AllocationResult to the specified file path.
     *
     * @details Iterates through the webs and their assigned registers to construct the final text file. 
     * If the allocation was completely unsuccessful according to the strict rules, it formats 
     * all webs to memory ('M') and sets registers to 0, printing a warning to `std::cerr`.
     * * <b>Time Complexity:</b> O(W * L), where W is the total number of webs and L is the maximum number of active lines per web.
     *
     * @param result     The final AllocationResult produced by the Allocator.
     * @param outputFile String representing the path to the output text file.
     * @throws std::runtime_error if the file cannot be opened for writing.
     */
    static void write(const AllocationResult& result, const std::string& outputFile);

private:
    /**
     * @brief Formats a single web's active program points into the required string format.
     *
     * @details Points are iterated in ascending order (guaranteed by the underlying `std::set`). 
     * The start point receives a '+' suffix, and the end point receives a '-' suffix. 
     * If a point is simultaneously a start and an end (edge case with fused ranges), the '+' takes precedence.
     * * <b>Time Complexity:</b> O(L), where L is the number of active lines in the web.
     *
     * @param web The web structure to format.
     * @return A comma-separated string of annotated line numbers.
     */
    static std::string formatWebPoints(const Web& web);
};

#endif // WRITER_H