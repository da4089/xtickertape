/* $Id: StringBuffer.h,v 1.3 1998/10/24 04:55:31 phelps Exp $
 * COPYRIGHT!
 *
 * Helps in the construction of Strings of unknown length by
 * guaranteeing that we never run out of buffer
 */

#ifndef STRING_BUFFER_H
#define STRING_BUFFER_H

typedef struct StringBuffer_t *StringBuffer;

/* Allocates an empty StringBuffer */
StringBuffer StringBuffer_alloc();

/* Frees the resources used by the receiver */
void StringBuffer_free(StringBuffer self);

/* Prints out debugging information about the receiver */
void StringBuffer_debug(StringBuffer self);

/* Resets the buffer to its first character */
void StringBuffer_clear(StringBuffer self);

/* Answers the number of characters in the receiver's buffer */
unsigned long StringBuffer_length(StringBuffer self);

/* Appends a single character to the receiver */
void StringBuffer_appendChar(StringBuffer self, char ch);

/* Appends an integer to the receiver */
void StringBuffer_appendInt(StringBuffer self, int value);

/* Appends a null-terminated character array to the receiver */
void StringBuffer_append(StringBuffer self, char *string);

/* Answers a pointer to the receiver's null-terminated character array */
char *StringBuffer_getBuffer(StringBuffer self);

#endif /* STRING_BUFFER_H */