#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "map.h"
#include <stdbool.h>

#define ILLEGAL -1

typedef struct Node_t* Node;

struct Node_t
{
    MapDataElement data;
    MapKeyElement key;
    Node next;
    Node prev;
};

struct Map_t
{
    copyMapKeyElements copy_key;
    copyMapDataElements copy_data;
    freeMapKeyElements free_key;
    freeMapDataElements free_data;
    compareMapKeyElements cmp_key;
    Node current;
    int size;
    Node head;
    Node tail;
};

/**
 * This function gets a map with iterator(current) pointing to the node that
 * we want to remove from the map, removes in then connects the previous and
 * the next nodes, then updates the size of the map.
 *
 * @param map
 */
static void nodeDestroy(Map map);

/**
 * this function gets a map with iterator(current) pointing to the node, which
 * we want to add the new node next to it, also gets the key and data
 * associated to the new node.
 *
 * @param map
 * @param keyElement
 * @param dataElement
 * @return :
 * MAP_OUT_OF_MEMORY if an allocation fails
 * MAP_SUCCESS if the node was added successfully
 */
static MapResult createNode(Map map, MapKeyElement keyElement,
                            MapDataElement dataElement);

Map mapCreate(copyMapDataElements copyDataElement,
              copyMapKeyElements copyKeyElement,
              freeMapDataElements freeDataElement,
              freeMapKeyElements freeKeyElement,
              compareMapKeyElements compareKeyElements)
{
    if((!copyDataElement)||(!copyKeyElement)||(!freeDataElement)||
       (!freeKeyElement)||(!compareKeyElements))
    {
        return NULL;
    }
    Map map=malloc(sizeof(*map));
    if(!map)
    {
        return NULL;
    }
    map->head=NULL;
    map->tail=NULL;
    map->copy_key=copyKeyElement;
    map->copy_data=copyDataElement;
    map->free_key=freeKeyElement;
    map->free_data=freeDataElement;
    map->cmp_key=compareKeyElements;
    map->head=malloc(sizeof(*(map->head)));
    map->tail=malloc(sizeof(*(map->tail)));
    if(!(map->tail) || !(map->head))
    {
        free(map->tail);
        free(map->head);
        free(map);
        return NULL;
    }
    map->tail->key=NULL;
    map->tail->data=NULL;
    map->head->key=NULL;
    map->head->data=NULL;
    map->head->next=map->tail;
    map->head->prev=NULL;
    map->tail->next=NULL;
    map->tail->prev=map->head;
    map->current=map->head;
    map->size=0;
    return map;
}

void mapDestroy(Map map)
{
    if(!map)
    {
        return;
    }
    mapClear(map);
    free(map->head);
    free(map->tail);
    free(map);
}

Map mapCopy(Map map)
{
    if(!map)
    {
        return NULL;
    }
    Map new_map=mapCreate(map->copy_data,map->copy_key,map->free_data,
                          map->free_key,map->cmp_key);
    if(!new_map)
    {
        return NULL;
    }
    if(!map->size)
    {
        return new_map;
    }
    new_map->current=malloc(sizeof(*(new_map->current)));
    if(!(new_map->current))
    {
        mapDestroy(new_map);
        return NULL;
    }
    map->current=map->head->next;
    new_map->head->next=new_map->current;
    new_map->current->prev=new_map->head;
    new_map->current->key=(map->copy_key)(map->current->key);
    new_map->current->data=(map->copy_data)(map->current->data);
    map->current=map->current->next;
    if(!new_map->current->key || !new_map->current->data)
    {
        mapDestroy(new_map);
        return  NULL;
    }
    while(map->current!=map->tail)
    {
        Node tmp=new_map->current;
        new_map->current=malloc(sizeof(*(new_map->current)));
        if(!(new_map->current))
        {
            mapDestroy(new_map);
            return NULL;
        }
        new_map->current->prev=tmp;
        tmp->next=new_map->current;
        new_map->current->key=(map->copy_key)(map->current->key);
        new_map->current->data=(map->copy_data)(map->current->data);
        if(!new_map->current->key || !new_map->current->data)
        {
            mapDestroy(new_map);
            return  NULL;
        }
        map->current=map->current->next;
    }
    new_map->current->next=new_map->tail;
    new_map->tail->prev=new_map->current;
    new_map->size=map->size;
    return new_map;
}

int mapGetSize(Map map)
{
    if(!map)
    {
        return ILLEGAL;
    }
    return map->size;
}

bool mapContains(Map map, MapKeyElement element)
{
    if((!map)||(!element))
    {
        return false;
    }
    map->current=map->head->next;
    while(map->current!=map->tail)
    {
        if(!(map->cmp_key)(map->current->key ,element))
        {
            return true;
        }
        map->current=map->current->next;
    }
    return false;
}

MapResult mapPut(Map map, MapKeyElement keyElement, MapDataElement dataElement)
{
    if(!map || !keyElement || !dataElement)
    {
        return MAP_NULL_ARGUMENT;
    }
    map->current=map->head;
    if(!map->size)
    {
        return createNode(map,keyElement,dataElement);
    }
    if((map->cmp_key)(map->head->next->key,keyElement)>0)
    {
        return createNode(map,keyElement,dataElement);
    }
    if((map->cmp_key)(map->tail->prev->key,keyElement)<0)
    {
        map->current=map->tail->prev;
        return createNode(map,keyElement,dataElement);
    }
    map->current=map->head->next;
    while(map->current!=map->tail->prev)
    {
        if(map->cmp_key(map->current->key,keyElement)==0)
        {
            map->free_data(map->current->data);
            map->current->data=map->copy_data(dataElement);
            if(!map->current->data)
            {
                return MAP_OUT_OF_MEMORY;
            }
        }
        if((map->cmp_key(map->current->key,keyElement)<0 &&
           map->cmp_key(map->current->next->key,keyElement)>0))
        {
            return createNode(map,keyElement,dataElement);
        }
        map->current=map->current->next;
    }
    return MAP_SUCCESS;
}

static MapResult createNode(Map map, MapKeyElement keyElement
        , MapDataElement dataElement)
{
    Node tmp=malloc(sizeof(*tmp));
    if(!tmp)
    {
        return MAP_OUT_OF_MEMORY;
    }
    tmp->next=map->current->next;
    tmp->prev=map->current;
    map->current->next->prev=tmp;
    map->current->next=tmp;
    tmp->key=map->copy_key(keyElement);
    tmp->data=map->copy_data(dataElement);
    if(!(tmp->key) || !(tmp->data) )
    {
        map->free_key(tmp->key);
        map->free_data(tmp->data);
        free(tmp);
        return MAP_OUT_OF_MEMORY;
    }
    map->size++;
    return MAP_SUCCESS;
}

MapDataElement mapGet(Map map, MapKeyElement keyElement)
{
    if(!map || !keyElement)
    {
        return NULL;
    }
    Node tmp=NULL;
    tmp=map->head->next;
    while(tmp!=map->tail)
    {
        if(map->cmp_key(tmp->key,keyElement)==0)
        {
            return tmp->data;
        }
        tmp=tmp->next;
    }
    return NULL;
}

MapResult mapRemove(Map map, MapKeyElement keyElement)
{
    if(!map || !keyElement)
    {
        return MAP_NULL_ARGUMENT;
    }
    map->current=map->head->next;
    while(map->current!=map->tail)
    {
        if(map->cmp_key(map->current->key,keyElement)==0)
        {
            nodeDestroy(map);
            return MAP_SUCCESS;
        }
        map->current=map->current->next;
    }
    return MAP_ITEM_DOES_NOT_EXIST;
}

MapKeyElement mapGetFirst(Map map)
{
    if(!map || map->size==0)
    {
        return NULL;
    }
    map->current=map->head->next;
    return map->current->key;
}

MapKeyElement mapGetNext(Map map)
{
    if(!map)
    {
        return NULL;
    }
    if(map->current==map->head || map->current==map->tail)
    {
        return NULL;
    }
    if(map->current->next!=map->tail)
    {
        map->current=map->current->next;
        return map->current->key;
    }
    return NULL;
}

MapResult mapClear(Map map)
{
    if(!map)
    {
        return MAP_NULL_ARGUMENT;
    }
    map->current=map->head->next;
    Node tmp=NULL;
    while((map->current) != (map->tail))
    {
        tmp=map->current->next;
        nodeDestroy(map);
        map->current=tmp;
    }
    map->head->next=map->tail;
    map->tail->prev=map->head;
    map->current=map->head;
    return MAP_SUCCESS;
}

static void nodeDestroy(Map map)
{
    map->free_key(map->current->key);
    map->free_data(map->current->data);
    map->current->prev->next=map->current->next;
    map->current->next->prev=map->current->prev;
    free(map->current);
    (map->size)--;
}




