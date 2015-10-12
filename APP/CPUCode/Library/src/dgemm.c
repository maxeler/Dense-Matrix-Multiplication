#include <stdio.h>
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

max_file_t* maxfile;
max_engine_t* engine;
max_actions_t* actionsIn;
max_actions_t* actionsOut[2];
const max_handle_t* aHandle;
const max_handle_t* bHandle;
const max_handle_t* cHandle;
double* aIn;
double* bIn;
double* cOut[2];

void dgemm_init(size_t mMax, size_t nMax, size_t kMax) {
	printf("Initializing maxfile...\n");
	maxfile = DGEMM_init();
	aHandle = max_get_handle_stream(maxfile, "A");
	bHandle = max_get_handle_stream(maxfile, "B");
	cHandle = max_get_handle_stream(maxfile, "C");

	printf("Loading engine...\n");
	engine = max_load(maxfile, "*");

	actionsIn     = max_actions_init(maxfile, NULL);
	actionsOut[0] = max_actions_init_explicit(maxfile);
	actionsOut[1] = max_actions_init_explicit(maxfile);

	size_t mTiles = sizeInTiles(mMax);
	size_t nTiles = sizeInTiles(nMax);
	size_t kTiles = sizeInTiles(kMax);
	aIn     = malloc(mTiles * kTiles * TILE_SIZE_2D * sizeof(double));
	bIn     = malloc(kTiles * nTiles * TILE_SIZE_2D * sizeof(double));
	cOut[0] = malloc(kTiles * TILE_SIZE_2D * sizeof(double));
	cOut[1] = malloc(kTiles * TILE_SIZE_2D * sizeof(double));
}

void dgemm(
		const char* trans_a, const char* trans_b, size_t m, size_t n, size_t k, double alpha,
		const double* A, size_t lda, const double* B, size_t ldb, double beta, double* C, size_t ldc)
{
	// TODO support transform

	size_t mTiles = sizeInTiles(m);
	size_t nTiles = sizeInTiles(n);
	size_t kTiles = sizeInTiles(k);

	// re-order A and B matrices into tiles and pad them up to size
	size_t pos = 0;
	for (size_t mm = 0; mm < mTiles; ++mm) {
		for (size_t kk = 0; kk < kTiles; ++kk) {
			for (size_t x = 0; x < TILE_SIZE; ++x) {
				size_t row = mm*TILE_SIZE + x;

				if (row < m) {
					for (size_t y = 0; y < TILE_SIZE; ++y) {
						size_t col = kk*TILE_SIZE + y;
						aIn[pos++] = (col < k) ? A[row*lda+col] : 0;
					}
				} else {
					for (size_t y = 0; y < TILE_SIZE; ++y) {
						aIn[pos++] = 0;
					}
				}
			}
		}
	}

	pos = 0;
	for (size_t nn = 0; nn < nTiles; ++nn) {
		for (size_t kk = 0; kk < kTiles; ++kk) {
			for (size_t x = 0; x < TILE_SIZE; ++x) {
				size_t row = kk*TILE_SIZE + x;

				if (row < k) {
					for (size_t y = 0; y < TILE_SIZE; ++y) {
						size_t col = nn*TILE_SIZE + y;
						bIn[pos++] = (col < n) ? B[row*ldb+col] : 0;
					}
				} else {
					for (size_t y = 0; y < TILE_SIZE; ++y) {
						bIn[pos++] = 0;
					}
				}
			}
		}
	}

	max_clear_queues(actionsIn);
	max_set_ticks(actionsIn, "TM", mTiles * nTiles * kTiles * TILE_SIZE_2D);

	for (size_t mm = 0; mm < mTiles; ++mm) {
		for (size_t nn = 0; nn < nTiles; ++nn) {
			max_queue_input_handle(actionsIn, aHandle, &aIn[mm*kTiles*TILE_SIZE_2D], kTiles * TILE_SIZE_2D * sizeof(double));
		}
	}

	for (size_t mm = 0; mm < mTiles; ++mm) {
		max_queue_input_handle(actionsIn, bHandle, bIn, kTiles * nTiles * TILE_SIZE_2D * sizeof(double));
	}

	max_disable_stream_sync(actionsIn, "A");
	max_disable_stream_sync(actionsIn, "B");

	max_run(engine, actionsIn);

	max_run_t* run[2];
	for (size_t i = 0; i < 2; ++i) {
		max_clear_queues(actionsOut[i]);
		max_queue_output_handle(actionsOut[i], cHandle, cOut[i], kTiles * TILE_SIZE_2D * sizeof(double));
		max_sync_stream(actionsOut[i], "C");

		run[i] = (mTiles * nTiles > i) ? max_run_nonblock(engine, actionsOut[i]) : NULL;
	}

	size_t tile = 2;
	for (size_t mm = 0; mm < mTiles; ++mm) {
		size_t xMax = min(TILE_SIZE, m - mm*TILE_SIZE);

		for (size_t nn = 0; nn < nTiles; ++nn, ++tile) {
			size_t sel  = tile & 1;
			size_t yMax = min(TILE_SIZE, n - nn*TILE_SIZE);

			for (size_t x = 0; x < xMax; ++x) {
				size_t row = mm*TILE_SIZE + x;

				for (size_t y = 0; y < yMax; ++y) {
					size_t col = nn*TILE_SIZE + y;
					C[row*ldc+col] *= beta;
				}
			}

			max_wait(run[sel]);

			for (size_t kk = 0; kk < kTiles; ++kk) {
				for (size_t x = 0; x < xMax; ++x) {
					size_t row = mm*TILE_SIZE + x;

					for (size_t y = 0; y < yMax; ++y) {
						size_t col = nn*TILE_SIZE + y;
						C[row*ldc+col] += alpha * cOut[sel][TILE_SIZE_2D*kk + TILE_SIZE*x + y];
					}
				}
			}

			run[sel] = (tile < mTiles*nTiles) ? max_run_nonblock(engine, actionsOut[sel]) : NULL;
		}
	}
}

