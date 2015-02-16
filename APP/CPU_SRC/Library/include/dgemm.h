#include <stdlib.h>

void dgemm_init(size_t mMax, size_t nMax, size_t kMax);
void dgemm(
		const char* trans_a, const char* trans_b, size_t m, size_t n, size_t k, double alpha,
		const double* A, size_t lda, const double* B, size_t ldb, double beta, double* C, size_t ldc);

