# hashtable-for-C
A simple, easy to understand hashtable in C, the keys are strings and it depends on spooky hash (included)

The license is public domain, which is also the license for the included spooky hash.

Read hashtable.h for detailed descriptions.

I am doing a little work on Tiny C compiler and I tool needed a hashtable and it had to be in C.

I tried a badly too fancy library off the web that used macros to try to implement some generic programming.  The result was unreadable code that turned out to be buggy.

This is the opposite of that.  It's easy to read and there are no macros.  The comments lay out all the details you'll need to know, how memory is managed, who owns what. It's been tested, though I left the test code out in order to avoid complicating this with dependencies.

I chose spooky hash because that's one of the few reasonably good hash functions that has portable C code.  This should compile on any C compiler, not just the popular ones.  It doesn't use any machine specific or operating system specific features.

It's been tested on GCC, Visual C and TCC.

The design is an array of entries (not of pointers to entries) but it handles collisions with external lists and calls to malloc.
However it is set to automatically expand when the number of entries reaches half of the size of the array, so it shouldn't have too many collisions. 

None of the code is thread safe.

There is a forward iterator. You can safely delete while iterating but only by using a delete function that deletes the item that the iterator is on, this advances the iterator. Any other operations on the hash table invalidate the iterator.

... Another part of this project includes generating a perfect hash (a perfect hash is one that takes a set list of items and generates a hash table that has no collisions for those items).  That works and uses the same kind of hashing.  Perhaps in the future I'll put in a function to convert one of these hash tables into a perfect hash.

... Another thing I could add in the future is an option to trust the hash function and never bother to actually compare strings.  The probablily that a 64 bit hash will have a false collision is low enough that I can imagine situations where you want the speed more than you want a guarantee against a probability of failure that's astronomically low.
