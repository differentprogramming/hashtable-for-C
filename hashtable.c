
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <memory.h>

#include "hashtable.h"

bool InitHashIterator(HashIterator *hi, HashTable *h)
{
	int i;
	hi->h = h;
	hi->pos = 0;
	hi->cur_entry = NULL;
	hi->cur_slot = 0;
	for (i = 0; i < hi->h->HASH_SIZE; ++i) if (hi->h->data[i].key != NULL) break;
	if (i == hi->h->HASH_SIZE) return false;//table is empty
	hi->cur_slot = i;
	hi->cur_entry = &hi->h->data[i];
	return true;
}

bool IncHashIterator(HashIterator *hi)
{
	if (HashIteratorAtEnd(hi)) return false;
	++hi->pos;
	if (HashIteratorAtEnd(hi)) {
		hi->cur_entry = NULL;
		hi->cur_slot = 0;
		return false;
	}
	if (NULL != hi->cur_entry->next) {
		hi->cur_entry = hi->cur_entry->next;
	}
	else {
		do {
			++hi->cur_slot;
		} while (hi->h->data[hi->cur_slot].key == NULL);
		hi->cur_entry = &hi->h->data[hi->cur_slot];
	}
	return true;
}

//returns true if p is now the pointer to the next entry
bool HashDeleteU(HashTable *h, HashEntry *p, bool save_key)
{
	--h->number_of_entries;
	if (!save_key) {
		free((void *)p->key);
		CleanUpHashValue(p->value);
	}
	if (p->prev == NULL) {//inline entry
		if (p->next == NULL) {//no next entry
			memset(p, 0, sizeof(HashEntry));
		}
		else {
			HashEntry *next = p->next;
			memcpy(p, next, sizeof(HashEntry));
			p->prev = NULL;
			if (NULL != next->next) next->next->prev = p;
			free(next);
			return true;
		}
	}
	else {
		p->prev->next = p->next;
		if (NULL != p->next) p->next->prev = p->prev;
		free(p);
	}
	return false;
}

bool _DeleteAtHashIterator(HashIterator *hi, bool save_key)
{
	if (HashIteratorAtEnd(hi)) return false;
	HashEntry *doomed = hi->cur_entry;
	int slot = hi->cur_slot;

	IncHashIterator(hi);
	if (HashDeleteU(hi->h, doomed, save_key)) {
		hi->cur_entry = doomed;
		hi->cur_slot = slot;
	}
	hi->pos--;
	return !HashIteratorAtEnd(hi);
}

bool DeleteAtHashIterator(HashIterator *hi)
{
	return _DeleteAtHashIterator(hi, false);
}

void InitHashTable(HashTable*h)
{
	h->number_of_entries = 0;
	h->HASH_SIZE = INITIAL_HASH_SIZE;
	h->data = calloc(INITIAL_HASH_SIZE, sizeof(HashEntry));
}

void DeleteHashTable(HashTable*h)
{
	if (h->data == NULL) return;
	HashIterator itr;
	InitHashIterator(&itr, h);
	while (DeleteAtHashIterator(&itr));
	free(h->data);
	h->data = NULL;
}

uint64_t hash_string(const char *s)
{
	return hash_string_with_seed(s, 0x5f71203b);
}


bool HashFindU(HashTable *h, HashEntry**ptr, uint64_t* hash_value, const char * s)
{
	uint64_t hs = hash_string(s);
	if (hash_value != NULL) *hash_value = hs;
	int at = (int)hs&(h->HASH_SIZE - 1);
	HashEntry* p = &h->data[at];
	if (ptr != NULL) *ptr = p;
	if (HashEmpty(h, at)) return false;
	while (p != NULL && (p->hash_value != hs || 0 != strncmp(s, p->key, MAX_HASH_STRING_SIZE))) {
		if (ptr != NULL) *ptr = p;
		p = p->next;
	}
	if (p != NULL) {
		if (ptr != NULL) *ptr = p;
		return true;
	}
	return false;
}
HashEntry* HashFind(HashTable *h, const char *s)
{
	HashEntry* p;
	if (HashFindU(h, &p, NULL, s)) {
		return p;
	}
	return NULL;
}

void ExpandHashTable(HashTable *h);

HashInsertStatus HashInsert(HashTable *h, HashEntry** ptr, const char *s, HashValueType v, bool replace)
{
	HashEntry *p;
	uint64_t hash_value;
	if (ptr == NULL) ptr = &p;
	if (HashFindU(h, ptr, &hash_value, s)) {
		if (replace) {
			CleanupHashValue((*ptr)->value);
			(*ptr)->value = v;
		}
		else CleanupHashValue(v); 
		return HS_Found;
	}
	else {
		HashEntry *prev = *ptr;
		if (prev->key == NULL)
		{
			prev = NULL;
		}
		else {
			(*ptr)->next = calloc(1, sizeof(HashEntry));
			*ptr = (*ptr)->next;
		}
		(*ptr)->prev = prev;
		(*ptr)->key = strdup(s);
		(*ptr)->value = v;
		(*ptr)->hash_value = hash_value;
		if (++h->number_of_entries >= h->HASH_SIZE >> 1) ExpandHashTable(h);
		return HS_Inserted;
	}
}

/* Assumes that the value is not in the table already and it copies the key without dupicating it. And it is handed the hash value instead of computing it.
 */
void _HashExpandInsert(HashTable *h, uint64_t hash_value, const char *s, HashValueType v)
{
	int at = (int)hash_value&(h->HASH_SIZE - 1);
	HashEntry* p = &h->data[at];
	if (!HashEmpty(h, at)) {
		while (p->next != NULL) p = p->next;
	}

	HashEntry *prev = p;
	if (prev->key == NULL)
	{
		prev = NULL;
	}
	else {
		p->next = calloc(1, sizeof(HashEntry));
		p = p->next;
	}
	p->prev = prev;
	p->key = s;
	p->value = v;
	p->hash_value = hash_value;
	++h->number_of_entries;
}


void ExpandHashTable(HashTable *h)
{
	HashTable temp;
	temp.number_of_entries = 0;
	temp.HASH_SIZE = h->HASH_SIZE << 1;
	temp.data = calloc(temp.HASH_SIZE, sizeof(HashEntry));
	HashIterator itr;
	InitHashIterator(&itr, h);
	while (!HashIteratorAtEnd(&itr)) {
		_HashExpandInsert(&temp, itr.cur_entry->hash_value, itr.cur_entry->key, itr.cur_entry->value);
		_DeleteAtHashIterator(&itr, true);
	};
	free(h->data);
	*h = temp;
}


bool HashDelete(HashTable *h, const char *s)
{
	HashEntry *p;
	if (!HashFindU(h, &p, NULL, s)) {
		return false;
	}
	HashDeleteU(h, p, false);
	return true;
}
