#ifndef JUDGE_H
#define JUDGE_H

#include "map.h"
#include <stdbool.h>

/**
* The following functions are available:
*   judgeMapCreate	- Creates a new empty map.
*   createJudge      - Creates a new judge node.
*   judgeDestroy	- Deletes an existing judge and frees all resources.
*   getJudgeResults        -returns the array that contains
                        the votes of the selected judge.
*/

/** Type for defining the judge */
typedef struct Judge_t *Judge;

/** Type used for returning error codes from judge functions */
typedef enum JudgeResult_t {
    JUDGE_SUCCESS,
    JUDGE_OUT_OF_MEMORY,
    JUDGE_NULL_ARGUMENT,
} JudgeResult;

/**
* judgeMapCreate: Allocates a new empty map.
*
* 	@return
 * 	NULL - if one of the parameters is NULL or allocations failed.
* 	A new Map in case of success.
*/
Map judgeMapCreate();

/**
* createJudge: Allocates a new judge and adds it to the given map.
 * the new judge contains the judge_name and judge_results
* @param map
* @param judge_id
* @param judge_name
* @param judge_results
* @return
* 	NULL - if one of the parameters is NULL or allocations failed.
* 	A new judge in case of success.
*/
Judge createJudge(Map map, int judge_id,const char *judge_name,
                  int *judge_results);

/**
* judgeDestroy: Deallocates an existing judge.
 * Clears all elements by using the
* stored free functions.
*
* @param judge - Target judge to be deallocated. If country is NULL
 * nothing will be done
*/
void judgeDestroy(Judge judge);

/**
*	getJudgeResults: Returns the votes array associated with a specific
 *	judge id in the given map that includes judges.
*
* @param map
* @return judge_id
*  NULL if a NULL was sent or if the map
 *  does not contain the requested judge_id.
* 	The country_name associated with the judge_id otherwise.
*/
int* getJudgeResults(Map map, int judge_id);

#endif //JUDGE_H
