#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "map.h"
#include "judge.h"
#include <stdbool.h>
#include <string.h>

#define SIZE_OF_RANKING_ARRAY 10

struct Judge_t
{
    char* judge_name;
    int* judge_results;
};


/**
* Type of function used by the map to copy key elements.
 * @param key
* This function should return:
* 		NULL if a null argument was sent or allocations failed
 * 		The copied key
*/
static MapKeyElement copyInt(MapKeyElement key) {
    if (!key) {
        return NULL;
    }
    int *copy = malloc(sizeof(*copy));
    if (!copy) {
        return NULL;
    }
    *copy = *(int *) key;
    return copy;
}


/**
* Type of function used by the map to copy a whole judge.
 * it allocates memory for the judge's name and votes array and
 * fills them according to the given country
 *
 * @param judge
* This function should return:
* 		NULL if a null argument was sent or allocations failed
 * 		The copied judge
*/
static MapDataElement copyDataJudge(MapDataElement judge) {
    if (!judge) {
        return NULL;
    }
    Judge copy = malloc(sizeof(*copy));
    if (!copy) {
        return NULL;
    }
    char* copyName=malloc(sizeof(char)*(strlen(((Judge)judge)->judge_name)+1));
    if(!copyName )
    {
        free(copyName);
        free(copy);
        return NULL;
    }
    strcpy(copyName,((Judge)judge)->judge_name);
    copy->judge_name=copyName;
    copy->judge_results=malloc(sizeof(int)*SIZE_OF_RANKING_ARRAY);
    if(!copy->judge_results)
    {
        free(copyName);
        free(copy);
        return NULL;
    }
    for(int i=0 ; i<SIZE_OF_RANKING_ARRAY ; i++)
    {
        *(copy->judge_results+i)=*(((Judge)judge)->judge_results+i);
    }
    return (MapDataElement) copy;
}

/**
* Type of function used by the map to to free the key element.
* @param key
*/
static void freeInt(MapKeyElement key) {
    free(key);
}

/**
* Type of function used by the map to to free judge
 * using the destroy function that clears the judge  .
* @param key
*/
static void freeJudge(MapDataElement judge) {
    judgeDestroy(judge);
}

/**
* Type of function used by the map to identify equal key elements.
* This function should return:
* 		A positive integer if the first element is greater;
* 		0 if they're equal;
*		A negative integer if the second element is greater.
*/
static int compareInts(MapKeyElement key1, MapKeyElement key2) {
    return (*(int *) key1 - *(int *) key2);
}

Map judgeMapCreate()
{
    return mapCreate(copyDataJudge, copyInt, freeJudge, freeInt, compareInts);
}

Judge createJudge(Map map, int judge_id, const char *judge_name,
                  int *judge_results)
{
    Judge judge = malloc(sizeof(*judge));
    if (!judge) {
        return NULL;
    }
    judge->judge_results=malloc(sizeof(int)*SIZE_OF_RANKING_ARRAY);
    if (!judge->judge_results) {
        free(judge);
        return NULL;
    }
    for(int i=0 ; i<SIZE_OF_RANKING_ARRAY ; i++)
    {
        *((judge->judge_results)+i)=*(judge_results+i);
    }
    char* copyName = malloc(sizeof(char) * (strlen(judge_name) + 1));
    if (!copyName) {
        judgeDestroy(judge);
        return NULL;
    }
    strcpy(copyName, judge_name);
    judge->judge_name = copyName;
    if (mapPut(map, &judge_id, judge)) {
        judgeDestroy(judge);
        return NULL;
    }
    judgeDestroy(judge);
    return judge;
}

void judgeDestroy(Judge judge)
{
    free(judge->judge_name);
    free(judge->judge_results);
    free(judge);
}

int* getJudgeResults(Map map, int judge_id)
{
    Judge judge=mapGet(map, &judge_id);
    if(!judge)
    {
        return NULL;
    }
    return judge->judge_results;
}



