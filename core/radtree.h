
#ifndef RADTREE_H
#define RADTREE_H

#include <sys/types.h>
#include <stdint.h>

struct radnode;

/**
 * The radix tree
 *
 * The elements are stored based on binary strings(0-255) of a given length.
 * They are sorted, a prefix is sorted before its suffixes.
 * If you want to know the key string, you should store it yourself, the
 * tree stores it in the parts necessary for lookup.
 * For binary strings for domain names see the radname routines.
 */
struct radtree {
	/** root node in tree */
	struct radnode* root;
	/** count of number of elements */
	size_t count;
};

/**
 * A radix tree lookup node.
 * The array is malloced separately from the radnode.
 */
struct radnode {
	/** data element associated with the binary string up to this node */
	void* elem;
	/** parent node (NULL for the root) */
	struct radnode* parent;
	/** index in the parent lookup array */
	uint8_t pidx;
	/** offset of the lookup array, add to [i] for lookups */
	uint8_t offset;
	/** length of the lookup array */
	uint16_t len;
	/** capacity of the lookup array (can be larger than length) */
	uint16_t capacity;
	/** the lookup array by [byte-offset] */
	struct radsel* array; 
};

/**
 * radix select edge in array
 */
struct radsel {
	/** additional string after the selection-byte for this edge. */
	uint8_t* str;
	/** length of the additional string for this edge */
	uint16_t len;
	/** node that deals with byte+str */
	struct radnode* node;
};

/**
 * Create new radix tree
 * @return new tree or NULL on alloc failure.
 */
struct radtree* radix_tree_create(void);


/**
 * Delete intermediate nodes from radix tree
 * @param rt: radix tree to be initialized.
 */
void radix_tree_clear(struct radtree* rt);

/**
 * Delete radix tree.
 * @param rt: radix tree to be deleted.
 */
void radix_tree_delete(struct radtree* rt);


/**
 * Insert element into radix tree.
 * @param rt: the radix tree.
 * @param key: key string.
 * @param len: length of key.
 * @param elem: pointer to element data.
 * @return NULL on failure - out of memory.
 * 	NULL on failure - duplicate entry.
 * 	On success the new radix node for this element.
 */
struct radnode* radix_insert(struct radtree* rt, uint8_t* k,
	uint16_t len, void* elem);

/**
 * Delete element from radix tree.
 * @param rt: the radix tree.
 * @param n: radix node for that element.
 * 	if NULL, nothing is deleted.
 */
void radix_delete(struct radtree* rt, struct radnode* n);

/**
 * Find radix element in tree.
 * @param rt: the radix tree.
 * @param key: key string.
 * @param len: length of key.
 * @return the radix node or NULL if not found.
 */
struct radnode* radix_search(struct radtree* rt, uint8_t* k,
	uint16_t len);

/**
 * Find radix element in tree, and if not found, find the closest smaller or
 * equal element in the tree.
 * @param rt: the radix tree.
 * @param key: key string.
 * @param len: length of key.
 * @param result: returns the radix node or closest match (NULL if key is
 * 	smaller than the smallest key in the tree).
 * @return true if exact match, false if no match.
 */
int radix_find_less_equal(struct radtree* rt, uint8_t* k, uint16_t len,
	struct radnode** result);

/**
 * Return the first (smallest) element in the tree.
 * @param rt: the radix tree.
 * @return: first node or NULL if none.
 */
struct radnode* radix_first(struct radtree* rt);

/**
 * Return the last (largest) element in the tree.
 * @param rt: the radix tree.
 * @return: last node or NULL if none.
 */
struct radnode* radix_last(struct radtree* rt);

/**
 * Return the next element.
 * @param n: the element to go from.
 * @return: next node or NULL if none.
 */
struct radnode* radix_next(struct radnode* n);

/**
 * Return the previous element.
 * @param n: the element to go from.
 * @return: prev node or NULL if none.
 */
struct radnode* radix_prev(struct radnode* n);

/*
 * Perform a walk through all elements of the tree.
 * node: variable of type struct radnode*.
 * tree: pointer to the tree.
 *	for(node=radix_first(tree); node; node=radix_next(node))
*/

/**
 * Create a binary string to represent a domain name
 * @param k: string buffer to store into
 * @param len: output length, initially, the max, output the result.
 * @param dname: the domain name to convert, in wireformat.
 * @param dlen: length of space for dname.
 */
void radomain_name_d2r(uint8_t* k, uint16_t* len, const uint8_t* dname,
	size_t dlen);

/**
 * Convert a binary string back to a domain name.
 * @param k: the binary string.
 * @param len: length of k.
 * @param dname: buffer to store domain name into.
 * @param dlen: length of dname (including root label).
 */
void radomain_name_r2d(uint8_t* k, uint16_t len, uint8_t* dname, size_t* dlen);

/**
 * Search the radix tree using a domain name.
 * The name is internally converted to a radname.
 * @param rt: tree
 * @param d: domain name, no compression pointers allowed.
 * @param max: max length to go from d.
 * @return NULL on parse error or if not found.
 */
struct radnode* radomain_name_search(struct radtree* rt, const uint8_t* d,
	size_t max);

/**
 * Find radix element in tree by domain name, and if not found,
 * find the closest smaller or equal element in the tree.
 * The name is internally converted to a radname (same sorting order).
 * @param rt: the radix tree.
 * @param d: domain name, no compression pointers allowed.
 * @param max: max length to go from d.
 * @param result: returns the radix node or closest match (NULL if key is
 * 	smaller than the smallest key in the tree).
 * 	could result in NULL on a parse error as well (with return false).
 * @return true if exact match, false if no match.
 */
int radomain_name_find_less_equal(struct radtree* rt, const uint8_t* d, size_t max,
	struct radnode** result);

/**
 * Insert radix element by domain name.
 * @param rt: the radix tree
 * @param d: domain name, no compression pointers.
 * @param max: max length from d.
 * @param elem: the element pointer to insert.
 * @return NULL on failure - out of memory.
 * 	NULL on failure - duplicate entry.
 * 	NULL on failure - parse error.
 * 	On success the radix node for this element.
 */
struct radnode* radomain_name_insert(struct radtree* rt, const uint8_t* d,
	size_t max, void* elem);

/**
 * Delete element by domain name from radix tree.
 * @param rt: the radix tree.
 * @param d: the domain name.  If it is not in the tree nothing happens.
 * @param max: max length.
 */
void radomain_name_delete(struct radtree* rt, const uint8_t* d, size_t max);

/** number of bytes in common in strings */
uint16_t bstr_common_ext(uint8_t* x, uint16_t xlen, uint8_t* y,
	uint16_t ylen);
/** true if one is prefix of the other */
int bstr_is_prefix_ext(uint8_t* p, uint16_t plen, uint8_t* x,
	uint16_t xlen);

#endif /* RADTREE_H */
