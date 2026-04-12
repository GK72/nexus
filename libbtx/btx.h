/**
 * Part of BTX Toolset.
 *
 * BTX C Bindings.
 *
 * @author  Gábor Krisztián Girhiny and Junie
 * @date    2026-04-12
 */

#ifndef BTX_C_H
#define BTX_C_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Structure containing the result of a BTX operation.
 */
typedef struct {
    uint8_t* data;      /**< Pointer to the resulting data (allocated on the heap). */
    size_t size;        /**< Size of the resulting data. */
    char* error;        /**< Pointer to an error message, or NULL if the operation was successful. */
} btx_result_t;

/**
 * @brief   Encodes a BTX text string into binary data.
 *
 * @param   input   Null-terminated BTX text string.
 *
 * @return  `btx_result_t` structure containing the binary data or an error
 *          message. The caller is responsible for freeing the result using
 *          btx_free_result.
 */
btx_result_t btx_encode(const char* input);

/**
 * @brief   Decodes binary data into a BTX text string.
 *
 * @param   data    Pointer to the binary data to decode.
 * @param   size    Size of the binary data.
 *
 * @return `btx_result_t` structure containing the BTX text string or an error
 *          message. The caller is responsible for freeing the result using
 *          btx_free_result. The decoded text is null-terminated in the 'data'
 *          field, and 'size' indicates its length (excluding the null-terminator).
 */
btx_result_t btx_decode(const uint8_t* data, size_t size);

/**
 * @brief   Frees the memory allocated for a `btx_result_t`.
 *
 * @param   result  The result structure to free.
 */
void btx_free_result(btx_result_t result);

#ifdef __cplusplus
}
#endif

#endif // BTX_C_H
