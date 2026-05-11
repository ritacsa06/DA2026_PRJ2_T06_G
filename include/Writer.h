#ifndef WRITER_H
#define WRITER_H

#include <string>
#include "Allocator.h"   // for AllocationResult

/**
 * @brief Writes the allocation result to a file in the required output format.
 *
 * Output format (see project spec, Figures 11 & 12):
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
 *
 * Time complexity: O(W * L) where W = number of webs, L = max active lines per web.
 */
class Writer {
public:
    /**
     * @brief Writes the AllocationResult to the given file path.
     *
     * If the allocation was unsuccessful (some webs spilled) a warning is
     * also printed to std::cerr.
     *
     * @param result     The result produced by Allocator::allocate().
     * @param outputFile Path to the output text file.
     * @throws std::runtime_error if the file cannot be opened for writing.
     */
    static void write(const AllocationResult& result, const std::string& outputFile);

private:
    /**
     * @brief Formats a single web's program points as the spec requires.
     *
     * Points are listed in ascending order.  The start point gets a '+' suffix
     * and the end point gets a '-' suffix.  If a point is both a start and an
     * end (edge case with fused ranges) the '+' takes precedence.
     *
     * @param web The web to format.
     * @return A comma-separated string of annotated line numbers.
     */
    static std::string formatWebPoints(const Web& web);
};

#endif // WRITER_H