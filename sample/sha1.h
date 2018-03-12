
#ifndef SHA1_H_
#define SHA1_H_

#include <stdint.h>
/** \def SHA1_HASH_BITS
 * definees the size of a SHA-1 hash in bits
 */

/** \def SHA1_HASH_BYTES
 * definees the size of a SHA-1 hash in bytes
 */

/** \def SHA1_BLOCK_BITS
 * definees the size of a SHA-1 input block in bits
 */

/** \def SHA1_BLOCK_BYTES
 * definees the size of a SHA-1 input block in bytes
 */
#define SHA1_HASH_BITS  160
#define SHA1_HASH_BYTES (SHA1_HASH_BITS/8)
#define SHA1_BLOCK_BITS 512
#define SHA1_BLOCK_BYTES (SHA1_BLOCK_BITS/8)

/** \typedef sha1_ctx_t
 * \brief SHA-1 context type
 *
 * A vatiable of this type may hold the state of a SHA-1 hashing process
 */
typedef struct {
    uint32_t h[5];
    uint64_t length;
} sha1_ctx_t;

/** \typedef sha1_hash_t
 * \brief hash value type
 * A variable of this type may hold a SHA-1 hash value
 */
/*
typedef uint8_t sha1_hash_t[SHA1_HASH_BITS/8];
*/

/** \fn sha1_init(sha1_ctx_t *state)
 * \brief initializes a SHA-1 context
 * This function sets a ::sha1_ctx_t variable to the initialization vector
 * for SHA-1 hashing.
 * \param state pointer to the SHA-1 context variable
 */
void sha1_init(sha1_ctx_t *state);

/** \fn sha1_nextBlock(sha1_ctx_t *state, const void *block)
 *  \brief process one input block
 * This function processes one input block and updates the hash context
 * accordingly
 * \param state pointer to the state variable to update
 * \param block pointer to the message block to process
 */
void sha1_nextBlock (sha1_ctx_t *state, const void *block);

/** \fn sha1_lastBlock(sha1_ctx_t *state, const void *block, uint16_t length_b)
 * \brief processes the given block and finalizes the context
 * This function processes the last block in a SHA-1 hashing process.
 * The block should have a maximum length of a single input block.
 * \param state pointer to the state variable to update and finalize
 * \param block pointer to themessage block to process
 * \param length_b length of the message block in bits
 */
void sha1_lastBlock (sha1_ctx_t *state, const void *block, uint16_t length_b);

/** \fn sha1_ctx2hash(sha1_hash_t *dest, sha1_ctx_t *state)
 * \brief convert a state variable into an actual hash value
 * Writes the hash value corresponding to the state to the memory pointed by dest.
 * \param dest pointer to the hash value destination
 * \param state pointer to the hash context
 */
void sha1_ctx2hash (void *dest, sha1_ctx_t *state);

/** \fn sha1(sha1_hash_t *dest, const void *msg, uint32_t length_b)
 * \brief hashing a message which in located entirely in RAM
 * This function automatically hashes a message which is entirely in RAM with
 * the SHA-1 hashing algorithm.
 * \param dest pointer to the hash value destination
 * \param msg  pointer to the message which should be hashed
 * \param length_b length of the message in bits
 */
void sha1(void *dest, const void *msg, uint32_t length_b);



#endif /*SHA1_H_*/
