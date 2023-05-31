#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "map.h"
#include "country.h"
#include "judge.h"
#include "eurovision.h"
#include <stdbool.h>
#include <string.h>

#define SIZE_OF_RANKING_ARRAY 10
#define ADD_VOTE 1
#define REMOVE_VOTE -1
#define FIRST_SCORE 12
#define SECOND_SCORE 10
#define FIRST 0
#define SECOND 1
#define ILLEGAL -1
#define TOP_TEN_COUNTRIES 10
#define USED -1
#define PERCENT 100
#define EXTRA 4

struct eurovision_t {
    Map judge_map;
    Map country_map;
};

/**
 * This function creates a new string and copies the original string on it.
 *
 * @param str
 * @return:
 * NULL if an allocation fails.
 * new copy of the original string in case of success.
 */
static ListElement copyString(ListElement str) {
    char *copy_string = malloc(sizeof(char) * (strlen(str) + 1));
    if (!copy_string) {
        return NULL;
    }
    return strcpy(copy_string, str);
}

/**
 * This function frees the string which the function receives.
 *
 * @param str
 */
static void freeString(ListElement str) {
    free(str);
}

/**
 * This function checks if the given string is legal, to be legal the string
 * must contain only small letters and spaces.
 *
 * @param str
 * @return:
 * true - return true if the string is legal
 * false - if the sstring doesnt meet the requirements
 */
static bool legalString(const char *str) {
    const char *tmp = str;
    while (*tmp) {
        if (*tmp == ' ' || (*tmp <= 'z' && *tmp >= 'a')) {
            tmp++;
            continue;
        }
        return false;
    }
    return true;
}

/**this function gets the eurovision struct and two state IDs, and a integer
 * which decides if we are removing a vote or adding one, then checks some
 * conditions that the inputs must satisfy, and then updates the votes
 * accordingly.
 *
 * @param eurovision - the eurivision struct we work with
 * @param stateGiver - the voting state
 * @param stateTaker - the state which the stategiver is voting for
 * @param vote - decides if we are adding a vote or removing one
 * @return :
 * EUROVISION_NULL_ARGUMENT if a NULL argument was received
 * EUROVISION_INVALID_ID if one or both of the IDs for the states is negative
 * EUROVISION_STATE_NOT_EXIST if one or both of the states aren't in the contest
 * EUROVISION_SAME_STATE if the state is voting for itslef
 * EUROVISION_OUT_OF_MEMORY in case a memory allocation failed
 * EUROVISION_SUCCESS in case of success
 */
static EurovisionResult voteUpdate(Eurovision eurovision, int stateGiver,
                                   int stateTaker, int vote) {
    int *tmp = NULL;
    if (!eurovision) {
        return EUROVISION_NULL_ARGUMENT;
    }
    if (stateGiver < 0 || stateTaker < 0) {
        return EUROVISION_INVALID_ID;
    }
    if (!mapContains(eurovision->country_map, &stateGiver) ||
        !mapContains(eurovision->country_map, &stateTaker)) {
        return EUROVISION_STATE_NOT_EXIST;
    }
    if (stateGiver == stateTaker) {
        return EUROVISION_SAME_STATE;
    }
    Map votes_map = getVotesMap(eurovision->country_map, stateGiver);
    if (!votes_map) {
        return EUROVISION_NULL_ARGUMENT;
    }
    if (mapContains(votes_map, &stateTaker)) {
        tmp = (int *) mapGet(votes_map, &stateTaker);
        *tmp += vote;
        if (*tmp < 0) {
            *tmp = 0;
        } else if (*tmp == 0) {
            mapRemove(votes_map, &stateTaker);
            return EUROVISION_SUCCESS;
        }
    } else if (vote == REMOVE_VOTE) {
        return EUROVISION_SUCCESS;
    } else if (mapPut(votes_map, &stateTaker, &vote) == MAP_OUT_OF_MEMORY) {
        eurovisionDestroy(eurovision);
        return EUROVISION_OUT_OF_MEMORY;
    }
    return EUROVISION_SUCCESS;
}

/**
 * this function checks if the judge has voted for the same state twice
 * @param judgeResults the judge ranking array
 * @return
 * true if the judge voted twice for the same state
 * false if the array is legal ( no double votes )
 */
static bool doubleJudgeVote(int *judgeResults) {
    for (int i = 0; i < SIZE_OF_RANKING_ARRAY; i++) {
        for (int j = i + 1; j < SIZE_OF_RANKING_ARRAY; j++) {
            if (*(judgeResults + i) == *(judgeResults + j)) {
                return true;
            }
        }
    }
    return false;
}

/**
 * this function gets a map of the votes of a specific country and finds the
 * country which gets the most votes.
 *
 * @param votes_map map which includes the countries and the votes of a
 * specific country to those countries
 * @return
 * country ID of the country with the maximum votes
 * ILLEGAL if the map is NULL or the map is empty
 */
static int findMax(Map votes_map) {
    if (!votes_map) {
        return ILLEGAL;
    }
    if (!mapGetSize(votes_map)) {
        return ILLEGAL;
    }
    int *max_country_id = mapGetFirst(votes_map);
    int *country_id = mapGetNext(votes_map);
    int country_votes, max_country_votes;
    while (country_id) {
        max_country_votes = *(int *) mapGet(votes_map, max_country_id);
        country_votes = *(int *) mapGet(votes_map, country_id);
        if (country_votes > max_country_votes) {
            max_country_id = country_id;
        }
        country_id = mapGetNext(votes_map);
    }
    return *max_country_id;
}

/**
 * this function counts the points each country gets from the audience and adds
 * the points to the audience score in the country struct, first the function
 * clears the scores for the new count, then for each country finds the top ten
 * and give them points accordingly.
 * @param eurovision
 * @return
 * EUROVISION_OUT_OF_MEMORY if an allocation failed
 * EUROVISION_SUCCESS if the scores were calculated successfully
 */
static EurovisionResult fillAudienceScore(Eurovision eurovision) {
    int *giver_country = mapGetFirst(eurovision->country_map);
    while (giver_country) {
        putAudienceScore(eurovision->country_map, *giver_country, 0);
        giver_country = mapGetNext(eurovision->country_map);
    }
    giver_country = mapGetFirst(eurovision->country_map);
    Map votes_map;
    int taker_country, updated_points;
    while (giver_country) {
        votes_map = getVotesMap(eurovision->country_map, *giver_country);
        Map votes_map_copy = mapCopy(votes_map);
        int votes_map_size = mapGetSize(votes_map);
        if (!votes_map) {
            return EUROVISION_OUT_OF_MEMORY;
        }
        for (int i = 0; i < TOP_TEN_COUNTRIES; i++) {
            if (!votes_map_size) {
                break;
            }
            votes_map_size--;
            taker_country = findMax(votes_map_copy);
            mapRemove(votes_map_copy, &taker_country);
            updated_points = getAudienceScore(eurovision->country_map,
                                              taker_country);
            if (i == FIRST) {
                updated_points += FIRST_SCORE;
            } else if (i == SECOND) {
                updated_points += SECOND_SCORE;
            } else {
                updated_points += (TOP_TEN_COUNTRIES - i);
            }
            putAudienceScore(eurovision->country_map, taker_country,
                             updated_points);
        }
        mapDestroy(votes_map_copy);
        giver_country = mapGetNext(eurovision->country_map);
    }
    return EUROVISION_SUCCESS;
}

/**
 * this function counts the points each country gets from the judges and adds
 * the points to the judges score in the country struct, first the function
 * clears the scores for the new count, then for each judge finds the top ten
 * and give them points accordingly.
 * @param eurovision
 */
static void fillJudgeScore(Eurovision eurovision) {
    int *giver_country = mapGetFirst(eurovision->country_map);
    while (giver_country) {
        putJudgesScore(eurovision->country_map, *giver_country, 0);
        giver_country = mapGetNext(eurovision->country_map);
    }
    int *giver_judge = mapGetFirst(eurovision->judge_map);
    int *votes_array;
    int taker_country, updated_points;
    while (giver_judge) {
        votes_array = getJudgeResults(eurovision->judge_map, *giver_judge);
        for (int i = 0; i < SIZE_OF_RANKING_ARRAY; i++) {
            taker_country = *(votes_array + i);
            updated_points = getJudgesScore(eurovision->country_map,
                                            taker_country);
            if (i == FIRST) {
                updated_points += FIRST_SCORE;
            } else if (i == SECOND) {
                updated_points += SECOND_SCORE;
            } else {
                updated_points += TOP_TEN_COUNTRIES - i;
            }
            putJudgesScore(eurovision->country_map, taker_country,
                           updated_points);
        }
        giver_judge = mapGetNext(eurovision->judge_map);
    }
}

/**
 * this function gets two country names and merge them in lexicographical order
 * into a new string : "name1 - name2"
 * @param first_country - name of the first country
 * @param second_country - name of the second coountry
 * @return
 * NULL if a memory allocation failed
 * string if the two names were merged successfully
 */
static char *combineNames(char *first_country, char *second_country) {
    char *tmp1 = first_country;
    char *tmp2 = second_country;
    char *space = " - ";
    char *friendly_countries = malloc(strlen(first_country)
                                      + strlen(second_country) + EXTRA);
    if (!friendly_countries) {
        return NULL;
    }

    while (*tmp1 && *tmp2 && *tmp1 == *tmp2) {
        tmp1++;
        tmp2++;
    }

    if (*tmp1 - *tmp2 > 0) {
        strcpy(friendly_countries, second_country);
        strcat(friendly_countries, space);
        strcat(friendly_countries, first_country);
    } else {
        strcpy(friendly_countries, first_country);
        strcat(friendly_countries, space);
        strcat(friendly_countries, second_country);
    }
    return friendly_countries;
}

/**
 * this function is used as a sorting function for the listSort.
 * @param str1
 * @param str2
 * @return
 * positive if str1 > str2
 * zero if str1 = str2
 * negative if str1 < str2
 */
static int stringSort(ListElement str1, ListElement str2) {
    return strcmp(str1, str2);
}

/**
 * this function finds the matching id which the function receives and remove it
 * from the given list
 * @param list
 * @param id
 */
static void removeElementFromList(List list, int *id) {
    int *ptr = listGetFirst(list);
    while (ptr) {
        if (*ptr == *id) {
            listRemoveCurrent(list);
            return;
        }
        ptr = listGetNext(list);
    }
}

/**
 * this function allocates memory for an integer and copies the given value in
 * it.
 * @param key
 * @return
 * NULL if a NULL argument was received or a memory allocation failed
 * a pointer for the new integer with the same value in case of success
 */
static ListElement copyInt(ListElement key) {
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
 * this function frees the memory allocated for the given integer key
 * @param key
 */
static void freeInt(ListElement key) {
    free(key);
}

Eurovision eurovisionCreate() {
    Eurovision eurovision = malloc(sizeof(*eurovision));
    if (!eurovision) {
        return NULL;
    }
    eurovision->judge_map = judgeMapCreate();
    eurovision->country_map = countryMapCreate();
    if (!eurovision->country_map || !eurovision->judge_map) {
        eurovisionDestroy(eurovision);
        return NULL;
    }
    return eurovision;
}

void eurovisionDestroy(Eurovision eurovision) {
    if (!eurovision) {
        return;
    }
    mapDestroy(eurovision->judge_map);
    mapDestroy(eurovision->country_map);
    eurovision->judge_map = NULL;
    eurovision->country_map = NULL;
    free(eurovision);
}

EurovisionResult eurovisionAddState(Eurovision eurovision, int stateId,
                                    const char *stateName,
                                    const char *songName) {
    if (!eurovision || !stateName || !songName) {
        return EUROVISION_NULL_ARGUMENT;
    }
    if (stateId < 0) {
        return EUROVISION_INVALID_ID;
    }
    if (!legalString(stateName) || !legalString(songName)) {
        return EUROVISION_INVALID_NAME;
    }
    if (mapContains(eurovision->country_map, &stateId)) {
        return EUROVISION_STATE_ALREADY_EXIST;
    }
    if (!createCountry(eurovision->country_map, stateId, stateName,
                       songName)) {
        eurovisionDestroy(eurovision);
        return EUROVISION_OUT_OF_MEMORY;
    }
    return EUROVISION_SUCCESS;
}

EurovisionResult eurovisionRemoveState(Eurovision eurovision, int stateId) {
    if (!eurovision) {
        return EUROVISION_NULL_ARGUMENT;
    }
    if (stateId < 0) {
        return EUROVISION_INVALID_ID;
    }
    if (!mapContains(eurovision->country_map, &stateId)) {
        return EUROVISION_STATE_NOT_EXIST;
    }
    mapRemove(eurovision->country_map, &stateId);
    int *judge_id = mapGetFirst(eurovision->judge_map), *judge_results;
    bool judge_removed = false;
    while (judge_id) {
        judge_results = getJudgeResults(eurovision->judge_map, *judge_id);
        for (int i = 0; i < SIZE_OF_RANKING_ARRAY; i++) {
            if (*(judge_results + i) == stateId) {
                mapRemove(eurovision->judge_map, judge_id);
                judge_removed = true;
                break;
            }
        }
        if (judge_removed) {
            judge_id = mapGetFirst(eurovision->judge_map);
            judge_removed = false;
        } else {
            judge_id = mapGetNext(eurovision->judge_map);
        }
    }
    int *country_id = mapGetFirst(eurovision->country_map);
    Map votes_map;
    while (country_id) {
        votes_map = getVotesMap(eurovision->country_map, *country_id);
        if (mapContains(votes_map, &stateId)) {
            mapRemove(votes_map, &stateId);
            country_id = mapGetFirst(eurovision->country_map);
        } else {
            country_id = mapGetNext(eurovision->country_map);
        }
    }
    return EUROVISION_SUCCESS;
}

EurovisionResult eurovisionAddJudge(Eurovision eurovision, int judgeId,
                                    const char *judgeName,
                                    int *judgeResults) {
    if (!eurovision || !judgeName || !judgeResults) {
        return EUROVISION_NULL_ARGUMENT;
    }
    if (judgeId < 0) {
        return EUROVISION_INVALID_ID;
    }
    if (doubleJudgeVote(judgeResults)) {
        return EUROVISION_INVALID_ID;
    }
    for (int i = 0; i < SIZE_OF_RANKING_ARRAY; i++) {
        if (*(judgeResults + i) < 0) {
            return EUROVISION_INVALID_ID;
        }
    }
    if (!legalString(judgeName)) {
        return EUROVISION_INVALID_NAME;
    }
    for (int i = 0; i < SIZE_OF_RANKING_ARRAY; i++) {
        if (!mapContains(eurovision->country_map, judgeResults + i)) {
            return EUROVISION_STATE_NOT_EXIST;
        }
    }
    if (mapContains(eurovision->judge_map, &judgeId)) {
        return EUROVISION_JUDGE_ALREADY_EXIST;
    }

    if (!createJudge(eurovision->judge_map, judgeId, judgeName,
                     judgeResults)) {
        eurovisionDestroy(eurovision);
        return EUROVISION_OUT_OF_MEMORY;
    }
    return EUROVISION_SUCCESS;
}

EurovisionResult eurovisionRemoveJudge(Eurovision eurovision, int judgeId) {
    if (!eurovision) {
        return EUROVISION_NULL_ARGUMENT;
    }
    if (judgeId < 0) {
        return EUROVISION_INVALID_ID;
    }
    if (!mapContains(eurovision->judge_map, &judgeId)) {
        return EUROVISION_JUDGE_NOT_EXIST;
    }
    mapRemove(eurovision->judge_map, &judgeId);
    return EUROVISION_SUCCESS;
}

EurovisionResult eurovisionAddVote(Eurovision eurovision, int stateGiver,
                                   int stateTaker) {
    return voteUpdate(eurovision, stateGiver, stateTaker, ADD_VOTE);
}

EurovisionResult eurovisionRemoveVote(Eurovision eurovision, int stateGiver,
                                      int stateTaker) {
    return voteUpdate(eurovision, stateGiver, stateTaker, REMOVE_VOTE);
}

List eurovisionRunAudienceFavorite(Eurovision eurovision) {
    if (fillAudienceScore(eurovision) == EUROVISION_OUT_OF_MEMORY) {
        eurovisionDestroy(eurovision);
        return NULL;
    }
    List audience_favorite = listCreate(copyString, freeString);
    if (!audience_favorite) {
        eurovisionDestroy(eurovision);
        return NULL;
    }
    int countries_num = mapGetSize(eurovision->country_map);
    int country_score, max_country_score;
    int *max_country_id, *country_id;
    for (; countries_num > 0; countries_num--) {
        max_country_id = mapGetFirst(eurovision->country_map);
        country_id = mapGetNext(eurovision->country_map);
        while (country_id) {
            country_score = getAudienceScore(eurovision->country_map,
                                             *country_id);
            max_country_score = getAudienceScore(eurovision->country_map,
                                                 *max_country_id);
            if (country_score > max_country_score) {
                max_country_id = country_id;
            }
            country_id = mapGetNext(eurovision->country_map);
        }
        putAudienceScore(eurovision->country_map, *max_country_id, USED);
        listInsertLast(audience_favorite,
                       getCountryName(eurovision->country_map,
                                      *max_country_id));
    }
    return audience_favorite;
}

List eurovisionRunContest(Eurovision eurovision, int audiencePercent) {
    if (audiencePercent > PERCENT || audiencePercent < 1) {
        return NULL;
    }
    if (fillAudienceScore(eurovision) == EUROVISION_OUT_OF_MEMORY) {
        eurovisionDestroy(eurovision);
        return NULL;
    }
    List final_score = listCreate(copyString, freeString);
    if (!final_score) {
        eurovisionDestroy(eurovision);
        return NULL;
    }
    if (!mapGetSize(eurovision->country_map)) {
        return final_score;
    }
    fillJudgeScore(eurovision);
    int size = mapGetSize(eurovision->country_map);
    int judge_num = mapGetSize(eurovision->judge_map), countries_num = size - 1;
    double audience_score, judge_score, max_audience_score, max_judge_score;
    int *max_country_id, *country_id;
    for (; size > 0; size--) {
        max_country_id = mapGetFirst(eurovision->country_map);
        country_id = mapGetNext(eurovision->country_map);
        while (country_id) {
            audience_score = ((double) (getAudienceScore
                    (eurovision->country_map, *country_id)) / countries_num)
                             * (double) audiencePercent / PERCENT;
            max_audience_score = ((double) (getAudienceScore
                    (eurovision->country_map, *max_country_id)) / countries_num)
                                 * (double) audiencePercent / PERCENT;
            if (judge_num) {
                max_judge_score = ((double) (getJudgesScore
                        (eurovision->country_map, *max_country_id)) / judge_num)
                                  * (double) (PERCENT - audiencePercent) /
                                  PERCENT;
                judge_score = ((double) (getJudgesScore
                        (eurovision->country_map, *country_id)) / judge_num)
                              * (double) (PERCENT - audiencePercent) / PERCENT;
            } else {
                max_judge_score = 0;
                judge_score = 0;
            }
            if (audience_score + judge_score >
                max_audience_score + max_judge_score) {
                max_country_id = country_id;
            }
            country_id = mapGetNext(eurovision->country_map);
        }
        putAudienceScore(eurovision->country_map, *max_country_id, USED);
        putJudgesScore(eurovision->country_map, *max_country_id, USED);
        if (listInsertLast(final_score, getCountryName(eurovision->country_map,
                                                       *max_country_id))) {
            listDestroy(final_score);
            eurovisionDestroy(eurovision);
            return NULL;
        }
    }
    return final_score;
}

/**
 * this function gets a list with the countries id for each two countries it
 * fetches their votes and checks if they are friendly countries, in case they
 * are it combines the names into one sorted string and inserts into the
 * friendly countries list.
 * @param eurovision
 * @param id_list countries ID list
 * @param friendly_country_list
 * @return
 * false if a memory allocation failed
 * true if everything went well
 */
static bool fillFriendlyCountries(Eurovision eurovision, List id_list, List
                                  friendly_country_list) {
    Map current_votes_map, next_votes_map;
    int *current_id = listGetFirst(id_list);
    int *next_id = listGetNext(id_list);
    while (current_id) {
        while (next_id) {
            current_votes_map = (getVotesMap(eurovision->country_map,
                                             *current_id));
            next_votes_map = (getVotesMap(eurovision->country_map, *next_id));
            if ((findMax(current_votes_map)== *(int *) next_id) &&
                (findMax(next_votes_map)== *(int *) current_id)) {
                char *first_country = getCountryName
                        (eurovision->country_map, *current_id);
                char *second_country = getCountryName
                        (eurovision->country_map, *next_id);
                char *friendly_country = combineNames(first_country,
                                                      second_country);
                if (!friendly_country) {
                    listDestroy(friendly_country_list);
                    listDestroy(id_list);
                    eurovisionDestroy(eurovision);
                    return false;
                }
                if (listInsertLast(friendly_country_list, friendly_country)) {
                    free(friendly_country);
                    listDestroy(friendly_country_list);
                    listDestroy(id_list);
                    eurovisionDestroy(eurovision);
                    return false;
                }
                free(friendly_country);
                removeElementFromList(id_list, current_id);
                removeElementFromList(id_list, next_id);
                current_id = listGetFirst(id_list);
            }
            next_id = listGetNext(id_list);
        }
        removeElementFromList(id_list, current_id);
        current_id = listGetFirst(id_list);
        next_id = listGetNext(id_list);
    }
    return true;
}

List eurovisionRunGetFriendlyStates(Eurovision eurovision) {
    if (!eurovision) {
        return NULL;
    }
    List friendly_country_list = listCreate(copyString, freeString);
    if (!friendly_country_list) {
        eurovisionDestroy(eurovision);
        return NULL;
    }
    if (!mapGetSize(eurovision->country_map)) {
        return friendly_country_list;
    }
    List id_list = listCreate(copyInt, freeInt);
    if (!id_list) {
        listDestroy(friendly_country_list);
        eurovisionDestroy(eurovision);
        return NULL;
    }
    int *current_id = mapGetFirst(eurovision->country_map);
    while (current_id) {
        if (listInsertLast(id_list, current_id)) {
            listDestroy(friendly_country_list);
            listDestroy(id_list);
            eurovisionDestroy(eurovision);
            return NULL;
        }
        current_id = mapGetNext(eurovision->country_map);
    }
    if(!fillFriendlyCountries(eurovision, id_list, friendly_country_list))
    {
        return NULL;
    }
    listDestroy(id_list);
    if (listSort(friendly_country_list, stringSort)) {
        listDestroy(friendly_country_list);
        eurovisionDestroy(eurovision);
        return NULL;
    }
    return friendly_country_list;
}

