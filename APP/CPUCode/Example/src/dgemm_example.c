#include <stdio.h>
#include <string.h>
#include <time.h>
#include <MaxSLiCInterface.h>
#include "Maxfiles.h"

#define TILE_SIZE DGEMM_tileSize
#define TILE_SIZE_2D (TILE_SIZE * TILE_SIZE)
#define FREQUENCY DGEMM_frequency

static size_t min(size_t a, size_t b) {
	return (a < b) ? a : b;
}

static size_t sizeInTiles(size_t n) {
	return (n + TILE_SIZE - 1) / TILE_SIZE;
}

#ifdef USE_BLAS
#include <cblas.h>
#endif

void dgemm_model(
		const char* trans_a, const char* trans_b, size_t m, size_t n, size_t k, double alpha,
		const double* A, size_t lda, const double* B, size_t ldb, double beta, double* C, size_t ldc)
{
	// TODO support transform

#ifdef USE_BLAS
	cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, m, n, k, alpha, A, lda, B, ldb, beta, C, ldc);
#else
	for (size_t mm = 0; mm < m; ++mm) {
		for (size_t nn = 0; nn < n; ++nn) {
			C[mm*ldc+nn] *= beta;
			for (size_t kk = 0; kk < k; ++kk) {
				C[mm*ldc+nn] += alpha * A[mm*lda+kk] * B[kk*ldb+nn];
			}
		}
	}
#endif
}

static struct timespec diff(struct timespec start, struct timespec finish) {
	struct timespec diff;
	if ((finish.tv_nsec < start.tv_nsec)) {
		diff.tv_sec  = finish.tv_sec  - start.tv_sec  - 1;
		diff.tv_nsec = finish.tv_nsec - start.tv_nsec + 1000000000;
	} else {
		diff.tv_sec  = finish.tv_sec  - start.tv_sec;
		diff.tv_nsec = finish.tv_nsec - start.tv_nsec;
	}

	return diff;
}

static int check_result(int m, int n, const double* expected, const double* actual) {
	for (int i = 0; i < m; ++i) {
		for (int j = 0; j < n; ++j) {
			int idx = i*n + j;
			if (expected[idx] != actual[idx]) {
				return 1;
			}
		}
	}
	return 0;
}

int main(int argc, char** argv) {
	unsigned seed = time(NULL);
	printf("Random seed: %u\n", seed);
	srand(seed);

	size_t m;
	size_t n;
	size_t k;

	if (argc == 1) {
		m = (random() % (5 * TILE_SIZE)) + TILE_SIZE;
		n = (random() % (5 * TILE_SIZE)) + TILE_SIZE;
		k = (random() % (5 * TILE_SIZE)) + TILE_SIZE;
	} else if (argc == 2) {
		m = n = k = atoi(argv[1]);
	} else {
		printf("Usage: %s [size]\n", argv[0]);
		exit(1);
	}

	double* A   = calloc(m * k, sizeof(double));
	double* B   = calloc(k * n, sizeof(double));
	double* Csw = calloc(m * n, sizeof(double));
	double* Chw = calloc(m * n, sizeof(double));

	printf("Matrix dimensions: m = %d, n = %d, k = %d\n", m, n, k);

	dgemm_init(m, n, k);

	size_t mTiles = sizeInTiles(m);
	size_t nTiles = sizeInTiles(n);
	size_t kTiles = sizeInTiles(k);

	printf("DFE tile size: %d\n", TILE_SIZE);
	printf("DFE compute dimensions: m = %zu, n = %zu, k = %zu\n",
			mTiles * TILE_SIZE, nTiles * TILE_SIZE, kTiles * TILE_SIZE);

	double cpuPoints = ((double) m) * n * k;
	double dfePoints = ((double) mTiles) * nTiles * kTiles * TILE_SIZE_2D;
	printf("DFE compute efficiency: %f\n", cpuPoints / (dfePoints * TILE_SIZE));
	printf("DFE frequency: %d MHz\n", FREQUENCY);
	// add one tile to account for stream offset TODO measure pipeline depth properly?
	printf("DFE predicted compute time: %f s\n", (dfePoints + TILE_SIZE_2D) / (((double) FREQUENCY) * 1e6));

	for (int i = 0; i < m*k; ++i) {
		A[i] = random() % 100;
	}

	for (int i = 0; i < k*n; ++i) {
		B[i] = random() % 100;
	}

	for (int i = 0; i < m*n; ++i) {
		Csw[i] = random() % 100;
		Chw[i] = Csw[i];
	}

	// TODO random
	double alpha = 1;
	double beta  = 0;

	struct timespec start;
	struct timespec finish;

	for (int xx = 0; xx < 5; ++xx) {
		printf("Repeat %d\n", xx);
		printf("Running HW... "); fflush(stdout);
		clock_gettime(CLOCK_REALTIME, &start);
		dgemm("n", "n", m, n, k, alpha, A, k, B, n, beta, Chw, n);
		clock_gettime(CLOCK_REALTIME, &finish);

		struct timespec hw_time  = diff(start, finish);

		printf("took: %ld.%09ld s\n", hw_time.tv_sec, hw_time.tv_nsec);
	}

	printf("Running SW... "); fflush(stdout);
	clock_gettime(CLOCK_REALTIME, &start);
	dgemm_model("n", "n", m, n, k, alpha, A, k, B, n, beta, Csw, n);
	clock_gettime(CLOCK_REALTIME, &finish);

	struct timespec sw_time = diff(start, finish);

	printf("took: %ld.%09ld s\n", sw_time.tv_sec, sw_time.tv_nsec);

	printf("Comparing results... ");
	int failed = check_result(m, n, Csw, Chw);
	printf("%s\n", failed ? "WRONG" : "CORRECT");
	printf("Done.\n");
	return failed;
}
