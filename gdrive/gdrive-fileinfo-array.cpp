
#include "gdrive-fileinfo-array.hpp"

#include <string.h>
//#include <exception>


using namespace std;
using namespace fusedrive;


/*************************************************************************
 * Private struct and declarations of private functions for use within 
 * this file
 *************************************************************************/

typedef struct Gdrive_Fileinfo_Array
{
    int nItems;
    int nMax;
    Fileinfo* pArray;
} Gdrive_Fileinfo_Array;

// No private functions


/*************************************************************************
 * Implementations of public functions for internal or external use
 *************************************************************************/

/******************
 * Constructors, factory methods, destructors and similar
 ******************/

Gdrive_Fileinfo_Array* gdrive_finfoarray_create(Gdrive& gInfo, int maxSize)
{
    Gdrive_Fileinfo_Array* pArray = (Gdrive_Fileinfo_Array*) malloc(sizeof(Gdrive_Fileinfo_Array));
    if (pArray != NULL)
    {
        size_t byteSize = maxSize * sizeof(Fileinfo);
        pArray->nItems = 0;
        pArray->nMax = maxSize;
        pArray->pArray = (Fileinfo*) malloc(byteSize);
        try
        {
            for (int i = 0; i < pArray->nMax; i++)
            {
                new(pArray->pArray + i) Fileinfo(gInfo);
            }
        }
        catch (const bad_alloc& e)
        {
            // Memory error
            free(pArray);
            return NULL;
        }
        //memset(pArray->pArray, 0, byteSize);
    }
    // else memory error, do nothing and return NULL (the value of pArray).
    
    // Return a pointer to the new struct.
    return pArray;
}

void gdrive_finfoarray_free(Gdrive_Fileinfo_Array* pArray)
{
    if (pArray == NULL)
    {
        // Nothing to do
        return;
    }
    
    for (int i = 0; i < pArray->nItems; i++)
    {
        (pArray->pArray + i)->gdrive_finfo_cleanup();
    }
    
    if (pArray->nItems > 0)
    {
        free(pArray->pArray);
    }
    
    // Not really necessary, but doesn't harm anything
    pArray->nItems = 0;
    pArray->pArray = NULL;
    
    free(pArray);
}


/******************
 * Getter and setter functions
 ******************/

const Fileinfo* 
gdrive_finfoarray_get_first(Gdrive_Fileinfo_Array* pArray)
{
    return (pArray->nItems > 0) ? pArray->pArray : NULL;
}

const Fileinfo* 
gdrive_finfoarray_get_next(Gdrive_Fileinfo_Array* pArray, 
                           const Fileinfo* pPrev)
{
    if (pArray == NULL || pPrev == NULL)
    {
        // Invalid arguments
        return NULL;
    }
    const Fileinfo* pEnd = pArray->pArray + pArray->nMax;
    const Fileinfo* pNext = pPrev + 1;
    return (pNext < pEnd) ? pNext : NULL;
}

int gdrive_finfoarray_get_count(Gdrive_Fileinfo_Array* pArray)
{
    return pArray->nItems;
}

/******************
 * Other accessible functions
 ******************/

int gdrive_finfoarray_add_from_json(Gdrive_Fileinfo_Array* pArray, 
                                        Gdrive_Json_Object* pObj)
{
    if (pArray == NULL || pObj == NULL)
    {
        // Invalid parameters
        return -1;
    }
    if (pArray->nItems >= pArray->nMax)
    {
        // Too many items
        return -1;
    }
    
    // Read the info in, and increment nItems to show the new count.
    (pArray->pArray + pArray->nItems++)->gdrive_finfo_read_json(pObj);
    
    return 0;
    
}


/*************************************************************************
 * Implementations of private functions for use within this file
 *************************************************************************/

// No private functions
