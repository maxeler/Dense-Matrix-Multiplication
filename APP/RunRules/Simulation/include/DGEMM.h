/**\file */
#ifndef SLIC_DECLARATIONS_DGEMM_H
#define SLIC_DECLARATIONS_DGEMM_H
#include "MaxSLiCInterface.h"
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define DGEMM_PCIE_ALIGNMENT (16)
#define DGEMM_tileSize (16)
#define DGEMM_frequency (150)


/*----------------------------------------------------------------------------*/
/*---------------------------- Interface default -----------------------------*/
/*----------------------------------------------------------------------------*/




/**
 * \brief Basic static function for the interface 'default'.
 * 
 * \param [in] ticks_TM The number of ticks for which kernel "TM" will run.
 * \param [in] instream_A Stream "A".
 * \param [in] instream_size_A The size of the stream instream_A in bytes.
 * \param [in] instream_B Stream "B".
 * \param [in] instream_size_B The size of the stream instream_B in bytes.
 * \param [out] outstream_C Stream "C".
 * \param [in] outstream_size_C The size of the stream outstream_C in bytes.
 */
void DGEMM(
	uint64_t ticks_TM,
	const void *instream_A,
	size_t instream_size_A,
	const void *instream_B,
	size_t instream_size_B,
	void *outstream_C,
	size_t outstream_size_C);

/**
 * \brief Basic static non-blocking function for the interface 'default'.
 * 
 * Schedule to run on an engine and return immediately.
 * The status of the run can be checked either by ::max_wait or ::max_nowait;
 * note that one of these *must* be called, so that associated memory can be released.
 * 
 * 
 * \param [in] ticks_TM The number of ticks for which kernel "TM" will run.
 * \param [in] instream_A Stream "A".
 * \param [in] instream_size_A The size of the stream instream_A in bytes.
 * \param [in] instream_B Stream "B".
 * \param [in] instream_size_B The size of the stream instream_B in bytes.
 * \param [out] outstream_C Stream "C".
 * \param [in] outstream_size_C The size of the stream outstream_C in bytes.
 * \return A handle on the execution status, or NULL in case of error.
 */
max_run_t *DGEMM_nonblock(
	uint64_t ticks_TM,
	const void *instream_A,
	size_t instream_size_A,
	const void *instream_B,
	size_t instream_size_B,
	void *outstream_C,
	size_t outstream_size_C);

/**
 * \brief Advanced static interface, structure for the engine interface 'default'
 * 
 */
typedef struct { 
	uint64_t ticks_TM; /**<  [in] The number of ticks for which kernel "TM" will run. */
	const void *instream_A; /**<  [in] Stream "A". */
	size_t instream_size_A; /**<  [in] The size of the stream instream_A in bytes. */
	const void *instream_B; /**<  [in] Stream "B". */
	size_t instream_size_B; /**<  [in] The size of the stream instream_B in bytes. */
	void *outstream_C; /**<  [out] Stream "C". */
	size_t outstream_size_C; /**<  [in] The size of the stream outstream_C in bytes. */
} DGEMM_actions_t;

/**
 * \brief Advanced static function for the interface 'default'.
 * 
 * \param [in] engine The engine on which the actions will be executed.
 * \param [in,out] interface_actions Actions to be executed.
 */
void DGEMM_run(
	max_engine_t *engine,
	DGEMM_actions_t *interface_actions);

/**
 * \brief Advanced static non-blocking function for the interface 'default'.
 *
 * Schedule the actions to run on the engine and return immediately.
 * The status of the run can be checked either by ::max_wait or ::max_nowait;
 * note that one of these *must* be called, so that associated memory can be released.
 *
 * 
 * \param [in] engine The engine on which the actions will be executed.
 * \param [in] interface_actions Actions to be executed.
 * \return A handle on the execution status of the actions, or NULL in case of error.
 */
max_run_t *DGEMM_run_nonblock(
	max_engine_t *engine,
	DGEMM_actions_t *interface_actions);

/**
 * \brief Group run advanced static function for the interface 'default'.
 * 
 * \param [in] group Group to use.
 * \param [in,out] interface_actions Actions to run.
 *
 * Run the actions on the first device available in the group.
 */
void DGEMM_run_group(max_group_t *group, DGEMM_actions_t *interface_actions);

/**
 * \brief Group run advanced static non-blocking function for the interface 'default'.
 * 
 *
 * Schedule the actions to run on the first device available in the group and return immediately.
 * The status of the run must be checked with ::max_wait. 
 * Note that use of ::max_nowait is prohibited with non-blocking running on groups:
 * see the ::max_run_group_nonblock documentation for more explanation.
 *
 * \param [in] group Group to use.
 * \param [in] interface_actions Actions to run.
 * \return A handle on the execution status of the actions, or NULL in case of error.
 */
max_run_t *DGEMM_run_group_nonblock(max_group_t *group, DGEMM_actions_t *interface_actions);

/**
 * \brief Array run advanced static function for the interface 'default'.
 * 
 * \param [in] engarray The array of devices to use.
 * \param [in,out] interface_actions The array of actions to run.
 *
 * Run the array of actions on the array of engines.  The length of interface_actions
 * must match the size of engarray.
 */
void DGEMM_run_array(max_engarray_t *engarray, DGEMM_actions_t *interface_actions[]);

/**
 * \brief Array run advanced static non-blocking function for the interface 'default'.
 * 
 *
 * Schedule to run the array of actions on the array of engines, and return immediately.
 * The length of interface_actions must match the size of engarray.
 * The status of the run can be checked either by ::max_wait or ::max_nowait;
 * note that one of these *must* be called, so that associated memory can be released.
 *
 * \param [in] engarray The array of devices to use.
 * \param [in] interface_actions The array of actions to run.
 * \return A handle on the execution status of the actions, or NULL in case of error.
 */
max_run_t *DGEMM_run_array_nonblock(max_engarray_t *engarray, DGEMM_actions_t *interface_actions[]);

/**
 * \brief Converts a static-interface action struct into a dynamic-interface max_actions_t struct.
 *
 * Note that this is an internal utility function used by other functions in the static interface.
 *
 * \param [in] maxfile The maxfile to use.
 * \param [in] interface_actions The interface-specific actions to run.
 * \return The dynamic-interface actions to run, or NULL in case of error.
 */
max_actions_t* DGEMM_convert(max_file_t *maxfile, DGEMM_actions_t *interface_actions);

/**
 * \brief Initialise a maxfile.
 */
max_file_t* DGEMM_init(void);

/* Error handling functions */
int DGEMM_has_errors(void);
const char* DGEMM_get_errors(void);
void DGEMM_clear_errors(void);
/* Free statically allocated maxfile data */
void DGEMM_free(void);
/* returns: -1 = error running command; 0 = no error reported */
int DGEMM_simulator_start(void);
/* returns: -1 = error running command; 0 = no error reported */
int DGEMM_simulator_stop(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* SLIC_DECLARATIONS_DGEMM_H */

