/* Probe for cifra's count_trailing_zeroes as it compiles on Windows. bitops.h computes it there as
 * 31 - _BitScanReverse(x), and _BitScanReverse finds the highest set bit, so the result reads as a
 * leading-zero count. This prints each answer beside the right one and exits 1 on any mismatch. It
 * is a probe: the workflow reports its numbers and does not let it fail the job. */
#include <stdio.h>

#include "bitops.h"

int main(void) {
	static const struct {
		uint32_t in;
		uint32_t want;
	} rows[] = { { 1u, 0u },           { 2u, 1u },           { 8u, 3u },         { 0x10u, 4u },
		         { 0x00ff0000u, 16u }, { 0x80000000u, 31u }, { 0x00000003u, 0u } };
	int bad = 0;
	size_t i;
	for (i = 0; i < sizeof rows / sizeof rows[0]; i++) {
		uint32_t got = count_trailing_zeroes(rows[i].in);
		printf("count_trailing_zeroes(0x%08x) = %u, want %u%s\n",
		       rows[i].in,
		       got,
		       rows[i].want,
		       got == rows[i].want ? "" : "  MISMATCH");
		if (got != rows[i].want)
			bad++;
	}
	printf("%d of %d rows wrong\n", bad, (int)(sizeof rows / sizeof rows[0]));
	return bad ? 1 : 0;
}
