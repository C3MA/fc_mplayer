//
//  proto.c
//  fc_c
//

#include "libvo/fc1/proto.h"
#include <math.h>

/**
 * @param[in] buffer
 * @param[in] offset where to start in the buffer
 * @param[out] pId the found id at the position in the buffer
 * @param[out] pType the found type at the position in the buffer
 * @return the new offset
 */
int parse(uint8_t *buffer, int offset, int* pId, int* pType)
{
    int in;
    offset = parse_number(buffer, offset, &in);
    // Error
    if (offset == -1) {
        return -1;
    }
    *pType = (in & 0x7);
    *pId = in>>3;
    return offset;
}

/*
 * @param[in] buffer
 * @param[in] offset
 * @param[in] id
 * @param[in] type
 * @return combined byte with both values
 */
int serialize(uint8_t *buffer, int offset, int id, int type)
{
    int out;
    // Error
    if (offset == -1) {
        return -1;
    }
    out = type & 0x7;
    out = out | (id << 3);
    offset = serialize_number(buffer, offset, out);
    return offset;
}

/*
 * @param[in] buffer
 * @param[in] offset
 * @param[out] value that was read
 * @return the new offset
 */
int parse_number(uint8_t *buffer, int offset, int *value)
{
    // Error
    if (offset == -1) {
        return -1;
    }
    int count = -1;
    *value=0;
    // check if the first bit is set (so the following byte must be read)
    do {
        count++;
        (*value) += pow(2, 7*count) * (buffer[offset+count] & 0x7F);
    } while(buffer[offset+count] & 0x80);
    
    return count+1+offset;
}

/*
 * @param[in] buffer
 * @param[in] offset
 * @param[out] value that will be written
 * @return the new offset
 */
// TODO was passiert bei value 0?
int serialize_number(uint8_t *buffer, int offset, int value)
{
    // Error
    if (offset == -1) {
        return -1;
    }
    do {
        buffer[offset] = value & 0x7F;
        value = value>>7;
        // always set the first bit to indicate more bytes
        buffer[offset] |= 0x80;
        offset++;
    } while (value > 0);
    
    // last byte ! -> correct the first bit
    buffer[offset - 1] = buffer[offset - 1] & 0x7F;
    
    return offset;
}
