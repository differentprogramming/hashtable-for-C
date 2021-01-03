#ifndef _HASH_TABLE_
#define _HASH_TABLE_
#include "spooky.h"

/* A simple hash table with the following assumptions:
1) they keys are strings up to length MAX_HASH_STRING_SIZE
a) the keys handed to the routines are constants and it's not up to the routines to own or clean them up.
b) strings will be copied into keys, and the Hash functions will manage that memory.
2) The table starts at INITIAL_HASH_SIZE and will grow as necessary to mostly avoid collisions
3) the value at a key is of type HashValueType and either has value semantics or the values passed to insert are already owned by the table and no copying is necessary;
a) to redefine the type, change the code in this include file
b) Supply a function CleanUpHashValue(HashValueType).  If it has value semantics, then make a function that does nothing, if it has other semantics then it deletes data.
c) note, if you Insert and a value is already at that spot, then whether it is replaced depends on the passed "replace" flag, and the value that is not in the table at the end
of the insert is cleaned up with CleanUpHashValue.
4) the code relies on spooky hash

The hash table uses external lists for collisions but the table stays at least as big as twice the number of elements so there shouldn't be too many collisions.
The elements of the hash table are not points they're embedded Entry structs, so not very much external calls to malloc and free will be necessary.
If you really want to avoid malloc and free, then you can change the test at the end of HashInsert to expand earlier.

 */

/* for safety string operations should have a maximum size, they truncate after that. */
#define MAX_HASH_STRING_SIZE 1051

#define INITIAL_HASH_SIZE 1024

/******************** User supply type and clean up code for value **********************************/
typedef int HashValueType;
extern void CleanUpHashValue(HashValueType v);
/***************************************************************/

typedef struct HashEntry_st {
	HashValueType  value;
	const char * key;
	struct HashEntry_st *prev;
	struct HashEntry_st *next;
	uint64_t hash_value;
} HashEntry;

typedef struct HashTable_st {
	int number_of_entries;
	int HASH_SIZE;
	HashEntry *data;
} HashTable;


/* Inserting or deleting from the hash table while you're iterating on it, makes the state of the iterator invalid and will cause bugs
 * except that it's ok to use DeleteAtHashIterator which will leave the iterator pointing to the next item if there is one.
 * note that the iterator can only move forward, although the data structures allow for backward motion as well, it's just not
 * implemented.
 */
typedef struct HashIterator_st {
	HashTable *h;
	HashEntry *cur_entry;
	int cur_slot;
	int pos;
} HashIterator;

static inline bool HashIteratorAtEnd(HashIterator *i)
{
	return i->pos == i->h->number_of_entries;
}

static inline uint64_t hash_string_with_seed(const char *s, uint64_t seed)
{
	size_t len = strlen(s);
	return spooky_hash64(s, (size_t)len, (uint64_t)len^seed);
}
/* has a standard seed for hash tables */
uint64_t hash_string(const char *s);


void InitHashTable(HashTable*h);

/* Note, if there are any items left in the table they will be cleaned up */
void DeleteHashTable(HashTable*h);

/* returns a HashEntry if the element is found, NULL otherwise */
HashEntry* HashFind(HashTable *h, const char *s);

/* returns false if the table is empty */
bool InitHashIterator(HashIterator *hi, HashTable *h);

/* returns true if the iterator is on an item, false when it's past all the items or if there are none */
bool IncHashIterator(HashIterator *hi);

/* leaves the iterator past what it deleted.  Returns false if the element deleted was at the end of the table or if the table was or is empty */
bool DeleteAtHashIterator(HashIterator *hi);

enum HashInsertStatus_e { HS_Inserted, HS_Found };
typedef enum HashInsertStatus_e HashInsertStatus;

/* returns HS_Inserted if no such key already existed in the table
 * returns HS_Found if that key was already in the table.
 * if replace is set then the value is replaced even if the key was already in the table.  
 * if replace is not set and the key is already in the table, then the value is not changed.
 *    in the case of a collision, then whichever value is not needed is cleaned up with CleanUpHashValue()
 *    if you need to avoid that, then test for a collision with HashFind before you call HashInsert.
 * if ptr is not NULL then *ptr is set to point at the new or found entry
 * note than the key is copied if it's placed in the table
 */
HashInsertStatus HashInsert(HashTable *h, HashEntry** ptr, const char *s, HashValueType v, bool replace);

/* deletes an element in the table, return false if no such key was found */
bool HashDelete(HashTable *h, const char *s);

#endif
