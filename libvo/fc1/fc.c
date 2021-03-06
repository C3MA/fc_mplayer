//
//  fc.c
//  fc_c
//
//

#include <stdio.h>
#include <stdlib.h>
#include "libvo/fc1/proto.h"
#include "libvo/fc1/fc.h"
#include <string.h>

/*
 * @param[in] pInput buffer with the protobuf content
 * @param[out] pOutput the buffer to fill with input and the corresponding header
 * @param[in] the length of the input protobuf data
 */
void add_header(uint8_t* pInput, uint8_t* pOutput, int lengthInput)
{
    char header[HEADER_LENGTH+2];
    sprintf(header, "%10d", lengthInput);
    memcpy(pOutput, header, HEADER_LENGTH);
    memcpy(pOutput+HEADER_LENGTH, pInput, lengthInput);
}

/*
 * @param[in] buffer
 * @param[in] offset
 * @param[out] sniptyp of the snip
 * @param[out] length of the snip
 * @return the new offset
 */
int get_header(uint8_t *buffer, int offset, int *sniptyp, int *length)
{
    int id, type;
    if (sscanf((char*)buffer, "%10d",length)!= 1) {
        return -1;
    }
    offset += HEADER_LENGTH;
    offset = parse(buffer, offset, &id, &type);
    if (id != SNIP_TYPE || type != PROTOTYPE_VARIANT)
        return -1;
    
    offset = parse_number(buffer, offset, sniptyp);
    
    return offset;
}

/*
 * @param[in] buffer
 * @param[in] offset
 * @param[in] proto_id of the variant
 * @param[in] value
 * @return the new offset
 */
int add_variant(uint8_t *buffer, int offset, int proto_id ,int value)
{
    // Error
    if (offset == -1) {
        return -1;
    }
    offset = serialize(buffer, offset, proto_id, PROTOTYPE_VARIANT);
    offset = serialize_number(buffer, offset, value);
    return offset;
}

/*
 * @param[in] value
 * @return number of Bytes for Value
 */
int variant_length(int proto_id ,int value)
{
    int n = 0;

    //Check if proto_id serialized more than one Byte is
    if (proto_id >> 7 != 0) {
        n += 1;
    }
    // calculate the numer of bytes for the Value
    do {
        value >>= 7;
        n++;
    } while (value != 0);
    return n;
}

/*
 * @param[in] buffer
 * @param[in] offset
 * @param[in] typ of the snip
 * @return the new offset
 */
int add_type(uint8_t *buffer, int offset, int typ)
{
    // Error
    if (offset == -1) {
        return -1;
    }
    offset = add_variant(buffer, offset, SNIP_TYPE, typ);
    return offset;
}


/*
 * @param[in|out] buffer
 * @param[in] offset
 * @param[in] proto_id
 * @param[in] data buffer
 * @param[in] legngth data
 * @return the new offset
 */
int add_lengthd(uint8_t *buffer, int offset, int proto_id ,uint8_t *data, long length)
{
    // Error
    if (offset == -1) {
        return -1;
    }
    offset = serialize(buffer, offset, proto_id, PROTOTYPE_LENGTHD);
    offset = serialize_number(buffer, offset, (int) length);
    memcpy(buffer+offset, data, length);
    offset +=  (int) length;
    return offset;
}

/*
 * @param[out] buffer
 * @param[in] offset
 * @param[in] proto_id
 * @return the new offset
 */
int add_lengthd_empty(uint8_t *buffer, int offset, int proto_id)
{
    // Error
    if (offset == -1) {
        return -1;
    }
    offset = serialize(buffer, offset, proto_id, PROTOTYPE_LENGTHD);
    if (offset != -1) {
        buffer[offset] = 0x00;
        offset++;
    }
    return offset;
}

/*
 * @param[out] buffer
 * @param[in] offset
 * @param[out] value the counter, to write
 * @return the new offset
 */
int send_ping(uint8_t *buffer, int offset, int counter)
{    
    offset = add_type(buffer, offset, SNIPTYPE_PING);
    
    /*
     * Write the header for Ping structure
     */
    offset = serialize(buffer, offset, SNIP_PINGSNIP, PROTOTYPE_LENGTHD);
    offset = serialize_number(buffer, offset, variant_length(PINGSNIP_COUNT, counter));
    
    /*
     * store the value into the buffer
     */
    offset = add_variant(buffer, offset, PINGSNIP_COUNT, counter);
    
    return offset;
}

/*
 * @param[in] buffer
 * @param[in] offset
 * @param[out] value the counter, that was read
 * @return the new offset
 */
int recv_ping(uint8_t *buffer, int offset, int *value)
{
    int id, type;
    offset = parse(buffer, offset, &id, &type);
    if (id != SNIP_PINGSNIP || type != PROTOTYPE_LENGTHD)
        return -1;
    
    offset = parse_number(buffer, offset, &type); // use type to store the length
    offset = parse(buffer, offset, &id, &type);
    if (id != PINGSNIP_COUNT || type != PROTOTYPE_VARIANT)
    {
        (*value) = -1;
        return offset;
    }
    else
    {
        int tmpOffset;
        tmpOffset = parse_number(buffer,offset, value);
        return tmpOffset;
    }
}

/*
 * @param[out] buffer
 * @param[in] offset
 * @param[in] value the counter, to write
 * @return the new offset
 */
int send_pong(uint8_t *buffer, int offset, int counter)
{    
    offset = add_type(buffer, offset, SNIPTYPE_PONG);
    
    /*
     * Write the header for Ping structure
     */
    offset = serialize(buffer, offset, SNIP_PONGSNIP, PROTOTYPE_LENGTHD);
    offset = serialize_number(buffer, offset, variant_length(PONGSNIP_COUNT, counter));
    
    /*
     * store the value into the buffer
     */
    offset = add_variant(buffer, offset, PONGSNIP_COUNT, counter);
    
    return offset;
}

/*
 * @param[in] buffer
 * @param[in] offset
 * @param[out] value the counter, that was read
 * @return the new offset
 */
int recv_pong(uint8_t *buffer, int offset, int *value)
{
    int id, type;
    offset = parse(buffer, offset, &id, &type);
    if (id != SNIP_PONGSNIP || type != PROTOTYPE_LENGTHD)
        return -1;
    
    offset = parse_number(buffer, offset, &type); // use type to store the length
    offset = parse(buffer, offset, &id, &type);
    if (id != PONGSNIP_COUNT || type != PROTOTYPE_VARIANT)
    {
        (*value) = -1;
        return offset;
    }
    else
    {
        int tmpOffset;
        tmpOffset = parse_number(buffer,offset, value);
        return tmpOffset;
    }
}

/*
 * @param[out] buffer
 * @param[in] offset
 * @param[in] color, string with color
 * @param[in] seqId, ID of the Sequence
 * @param[in] meta, buffer with Binarysequenzemetadta
 * @return the new offset
 */
int send_request(uint8_t *buffer, int offset, char *color, int seqId, uint8_t *meta, int length_meta)
{    
    long color_length = strlen(color);
    offset = add_type(buffer, offset, SNIPTYPE_REQUEST);
    
    offset = serialize(buffer, offset, SNIP_REQUESTSNIP, PROTOTYPE_LENGTHD);
    // calculate length of SNIP_Requestsnip
	int size = variant_length(REQUESTSNIP_META, length_meta) + length_meta + 1 /* The byte for the header */;
	size += variant_length(REQUESTSNIP_COLOR, (int)color_length) + color_length + 1 /* The byte for the header */;
	size += variant_length(REQUESTSNIP_SEQID, seqId) + 1 /* The byte for the header */;
    offset = serialize_number(buffer, offset, size);
    
    offset = add_lengthd(buffer, offset, REQUESTSNIP_COLOR, (uint8_t*) color, color_length);
    
    offset = add_variant(buffer, offset, REQUESTSNIP_SEQID, seqId);
    
    offset = add_lengthd(buffer, offset, REQUESTSNIP_META, meta, length_meta);
    
    return offset;
}

/*
 * @param[out] buffer
 * @param[in] offset
 * @param[in] frames_per_second
 * @param[in] width
 * @param[in] height
 * @param[in] generator_name
 * @param[in] generator_version 
 * @return the new offset
 */
int create_metadata(uint8_t *buffer, int offset, int frames_per_second, int width, int heigtht, char *generator_name, char *generator_version)
{    
    offset = add_variant(buffer, offset, BINARYSEQUENCEMETADATA_FRAMESPERSECOND, frames_per_second);
    
    offset = add_variant(buffer, offset, BINARYSEQUENCEMETADATA_WIDTH, width);
    offset = add_variant(buffer, offset, BINARYSEQUENCEMETADATA_HEIGHT, heigtht);
    
    offset = add_lengthd(buffer, offset, BINARYSEQUENCEMETADATA_GENERATORNAME, (uint8_t*) generator_name, strlen(generator_name));
    
    offset = add_lengthd(buffer, offset, BINARYSEQUENCEMETADATA_GENERATORVERSION, (uint8_t*) generator_version, strlen(generator_version));
    
    return offset;
}

/*
 * @param[in] meta
 * @param[out] frames per second (fps)
 * @param[out] width
 * @param[out] height
 * @param[out] generator name, pointer to memory area of generator name [YOU have to FREE this Memory later!1!]
 * @param[out] generator version, pointer of memory area of generator version [YOU have to FREE this Memory later!1!]
 * @return amount of parsed bytes
 */
int parse_metadata(uint8_t *buffer, int offset, int *frames_per_second, int *width, int *height, char **generator_name, char **generator_version)
{
    int id, type, length;
    
    // Read fps
    offset = parse(buffer, offset, &id, &type); // Read first byte and check if Right snip
    if (id != BINARYSEQUENCEMETADATA_FRAMESPERSECOND || type != PROTOTYPE_VARIANT)
        return -1;
    offset = parse_number(buffer, offset, frames_per_second); // Read value of fps
    
    // Read width
    offset = parse(buffer, offset, &id, &type); // Read first byte and check if Right snip
    if (id != BINARYSEQUENCEMETADATA_WIDTH || type != PROTOTYPE_VARIANT)
        return -1;
    offset = parse_number(buffer, offset, width); // Read value of width
    
    // Read height
    offset = parse(buffer, offset, &id, &type); // Read first byte and check if Right snip
    if (id != BINARYSEQUENCEMETADATA_HEIGHT || type != PROTOTYPE_VARIANT)
        return -1;
    offset = parse_number(buffer, offset, height); // Read value of heigth
    
    // Read generator_name
    offset = parse(buffer, offset, &id, &type);
    if (id == BINARYSEQUENCEMETADATA_GENERATORNAME && type == PROTOTYPE_LENGTHD && offset > -1)
    {
        offset = parse_number(buffer, offset, &length);
        *generator_name = (char*) malloc((long) length+1);   // +1 for 0x00 (string end)
        memcpy(*generator_name, buffer+offset, (long) length);
        (*generator_name)[length] = 0x00; // string ende
        offset += length;
    }
    else
    {
        return -1;
    }
    
    // Read generator_version
    offset = parse(buffer, offset, &id, &type);
    if (id == BINARYSEQUENCEMETADATA_GENERATORVERSION && type == PROTOTYPE_LENGTHD  && offset > -1)
    {
        offset = parse_number(buffer, offset, &length);
        *generator_version = (char*) malloc((long) length+1);   // +1 for 0x00 (string end)
        memcpy(*generator_version, buffer+offset, (long) length);
        (*generator_version)[length] = 0x00; // string ende
        offset += length;
    }
    else
    {
        return -1;
    }
    
    return offset;
}

/*
 * @param[in] buffer
 * @param[in] offset
 * @param[out] color, pointer to memory area of color [YOU have to FREE this Memory later!1!]
 * @param[out] seqId
 * @param[out] offset of the metadata
 * @param[out] meta_length, length of the Metadata
 * @return the new offset
 */
int recv_request(uint8_t *buffer, int offset, char **color, int *seqId, int *meta_offset, int *meta_length)
{
    int id, type, length;
    offset = parse(buffer, offset, &id, &type); // Read first byte and check if Right snip
    if (id != SNIP_REQUESTSNIP || type != PROTOTYPE_LENGTHD)
        return -1;
    
    offset = parse_number(buffer, offset, &length); // Read length of req_snip
    
     // Read Color
    
    offset = parse(buffer, offset, &id, &type);
    if (id == REQUESTSNIP_COLOR && type == PROTOTYPE_LENGTHD && offset > -1)
    {
        offset = parse_number(buffer, offset, &length);
        *color = (char*) malloc((long) length+1);   // +1 for 0x00 (string end)
        memcpy(*color, buffer+offset, (long) length);
        (*color)[length] = 0x00; // string ende
        offset += length;
    }
    else
    {
        return -1;
    }
    
    // Read SeqID
    
    offset = parse(buffer, offset, &id, &type); // Read first byte and check if Right snip
    if (id != REQUESTSNIP_SEQID || type != PROTOTYPE_VARIANT)
        return -1;
    offset = parse_number(buffer, offset, seqId); // Read value of SeqId
    
    // Read Metadata
    
    offset = parse(buffer, offset, &id, &type);  // Read Color
    if (id == REQUESTSNIP_META && type == PROTOTYPE_LENGTHD && offset > -1)
    {
        offset = parse_number(buffer, offset, meta_length);
        (*meta_offset) = offset;
        offset += (*meta_length);
    }
    else
    {
        return -1;
    }

    return offset;
}


/*
 * @param[out] buffer
 * @param[in] offset
 * @return the new offset
 */
int send_start(uint8_t *buffer, int offset)
{
    offset = add_type(buffer, offset, SNIPTYPE_START);
    offset = add_lengthd_empty(buffer, offset, SNIP_STARTSNIP);
    return offset;
}

/*
 * @param[in] buffer
 * @param[in] offset
 * @return the new offset
 */
int recv_start(uint8_t *buffer, int offset)
{
    int id, type, length;
    
    offset = parse(buffer, offset, &id, &type);
    if (id != SNIP_STARTSNIP || type != PROTOTYPE_LENGTHD)
        return -1;
    offset = parse_number(buffer, offset, &length);
    return offset;
}


/*
 * @param[out] buffer
 * @param[in] offset
 * @param[in] red
 * @param[in] green
 * @param[in] blue
 * @param[in] x
 * @param[in] y
 * @return the new offset
 */
int frame_add_pixel(uint8_t *buffer, int offset, int red, int green, int blue, int x, int y)
{    
    int lenght_pixel;
    /*
     * Write the header for pixel structure
     */
    offset = serialize(buffer, offset, BINARYFRAME_PIXEL, PROTOTYPE_LENGTHD);
    lenght_pixel = variant_length(RGBVALUE_RED, red) + 1 + variant_length(RGBVALUE_GREEN, green) + 1 + variant_length(RGBVALUE_BLUE, blue) + 1 + variant_length(RGBVALUE_X, x) + 1 + variant_length(RGBVALUE_Y, y) + 1;
    offset = serialize_number(buffer, offset, lenght_pixel);

    /*
     * store the value into the buffer
     */
    offset = add_variant(buffer, offset, RGBVALUE_RED, red);
    offset = add_variant(buffer, offset, RGBVALUE_GREEN, green);
    offset = add_variant(buffer, offset, RGBVALUE_BLUE, blue);
    offset = add_variant(buffer, offset, RGBVALUE_X, x);
    offset = add_variant(buffer, offset, RGBVALUE_Y, y);
    return offset;
}


/*
 * @param[in] buffer
 * @param[in] offset
 * @param[out] red
 * @param[out] green
 * @param[out] blue
 * @param[out] x
 * @param[out] y
 * @return the new offset
 */
int frame_parse_pixel(uint8_t *buffer, int offset, int *red, int *green, int *blue, int *x, int *y)
{
    int id, type, length;
    
    offset = parse(buffer, offset, &id, &type);
    if (id != BINARYFRAME_PIXEL || type != PROTOTYPE_LENGTHD)
        return -1;
    offset = parse_number(buffer, offset, &length);
    
    // Read red
    offset = parse(buffer, offset, &id, &type); // Read first byte and check if Right snip
    if (id != RGBVALUE_RED || type != PROTOTYPE_VARIANT)
        return -1;
    offset = parse_number(buffer, offset, red); // Read value of red
    
    // Read green
    offset = parse(buffer, offset, &id, &type); // Read first byte and check if Right snip
    if (id != RGBVALUE_GREEN || type != PROTOTYPE_VARIANT)
        return -1;
    offset = parse_number(buffer, offset, green); // Read value of green
    
    // Read blue
    offset = parse(buffer, offset, &id, &type); // Read first byte and check if Right snip
    if (id != RGBVALUE_BLUE || type != PROTOTYPE_VARIANT)
        return -1;
    offset = parse_number(buffer, offset, blue); // Read value of blue
    
    // Read x
    offset = parse(buffer, offset, &id, &type); // Read first byte and check if Right snip
    if (id != RGBVALUE_X || type != PROTOTYPE_VARIANT)
        return -1;
    offset = parse_number(buffer, offset, x); // Read value of x
    
    // Read y
    offset = parse(buffer, offset, &id, &type); // Read first byte and check if Right snip
    if (id != RGBVALUE_Y || type != PROTOTYPE_VARIANT)
        return -1;
    offset = parse_number(buffer, offset, y); // Read value of y
    
    return offset;
}

/*
 * @param[out] buffer
 * @param[in] offset
 * @param[in] frames buffer with frame
 * @param[in] length_frames length of buffer
 * @return the new offset
 */
int send_frame(uint8_t *buffer, int offset, uint8_t *frame, long length_frame)
{
    int lenght_frame_length;
    uint8_t length_frame_serialized[10]; //TODO: Größe ermitteln
    
    offset = add_type(buffer, offset, SNIPTYPE_FRAME);
    
    lenght_frame_length = serialize_number(length_frame_serialized, 0, (int)length_frame);
    
    offset = serialize(buffer, offset, SNIP_FRAMESNIP, PROTOTYPE_LENGTHD);
    /*
     * Add header for Frames, with two lenght values. Calculate first with length + length of next header :-/
     */
    offset = serialize_number(buffer, offset, (int)length_frame + lenght_frame_length + 1);
    
    offset = add_lengthd(buffer, offset, FRAMESNIP_FRAME, frame, (long)length_frame);
    
    return offset;
}

/*
 * @param[in] buffer
 * @param[in] offset
 * @param[out] offset of frame
 * @param[out] frame_length, length of the Framedata
 * @return the new offset
 */
int recv_frame(uint8_t *buffer, int offset, int *frame_offset, int *frame_length)
{
    int id, type, length;
    
    offset = parse(buffer, offset, &id, &type);
    if (id != SNIP_FRAMESNIP || type != PROTOTYPE_LENGTHD)
        return -1;
    
    offset = parse_number(buffer, offset, &length);
    
    offset = parse(buffer, offset, &id, &type);
    if (id == FRAMESNIP_FRAME && type == PROTOTYPE_LENGTHD)
    {
        offset = parse_number(buffer, offset, frame_length);
        (*frame_offset) = offset;
        offset += (*frame_length);
    }
    else
    {
        return -1;
    }
    
    return offset;
}

/*
 * @param[out] buffer
 * @param[in] offset
 * @return the new offset
 */
int send_ack(uint8_t *buffer, int offset)
{
    offset = add_type(buffer, offset, SNIPTYPE_ACK);
    offset = add_lengthd_empty(buffer, offset, SNIP_ACKSNIP);
    return offset;
}

/*
 * @param[in] buffer
 * @param[in] offset
 * @return the new offset
 */
int recv_ack(uint8_t *buffer, int offset)
{
    int id, type, length;
    
    offset = parse(buffer, offset, &id, &type);
    if (id != SNIP_ACKSNIP || type != PROTOTYPE_LENGTHD)
        return -1;
    offset = parse_number(buffer, offset, &length);
    return offset;
}

/*
 * @param[out] buffer
 * @param[in] offset
 * @return the new offset
 */
int send_nack(uint8_t *buffer, int offset)
{
    offset = add_type(buffer, offset, SNIPTYPE_NACK);
    offset = add_lengthd_empty(buffer, offset, SNIP_NACKSNIP);
    return offset;
}

/*
 * @param[in] buffer
 * @param[in] offset
 * @return the new offset
 */
int recv_nack(uint8_t *buffer, int offset)
{
    int id, type, length;
    
    offset = parse(buffer, offset, &id, &type);
    if (id != SNIP_NACKSNIP || type != PROTOTYPE_LENGTHD)
        return -1;
    offset = parse_number(buffer, offset, &length);
    return offset;
}

/*
 * @param[out] buffer
 * @param[in] offset
 * @return the new offset
 */
int send_timeout(uint8_t *buffer, int offset)
{
    offset = add_type(buffer, offset, SNIPTYPE_TIMEOUT);
    offset = add_lengthd_empty(buffer, offset, SNIP_TIMEOUTSNIP);
    return offset;
}

/*
 * @param[in] buffer
 * @param[in] offset
 * @return the new offset
 */
int recv_timeout(uint8_t *buffer, int offset)
{
    int id, type, length;
    
    offset = parse(buffer, offset, &id, &type);
    if (id != SNIP_TIMEOUTSNIP || type != PROTOTYPE_LENGTHD)
        return -1;
    offset = parse_number(buffer, offset, &length);
    return offset;
}

/*
 * @param[out] buffer
 * @param[in] offset
 * @return the new offset
 */
int send_abort(uint8_t *buffer, int offset)
{
    offset = add_type(buffer, offset, SNIPTYPE_ABORT);
    offset = add_lengthd_empty(buffer, offset, SNIP_ABORTSNIP);
    return offset;
}

/*
 * @param[in] buffer
 * @param[in] offset
 * @return the new offset
 */
int recv_abort(uint8_t *buffer, int offset)
{
    int id, type, length;
    
    offset = parse(buffer, offset, &id, &type);
    if (id != SNIP_ABORTSNIP || type != PROTOTYPE_LENGTHD)
        return -1;
    offset = parse_number(buffer, offset, &length);
    return offset;
}

/*
 * @param[out] buffer
 * @param[in] offset
 * @return the new offset
 */
int send_eos(uint8_t *buffer, int offset)
{
    offset = add_type(buffer, offset, SNIPTYPE_EOS);
    offset = add_lengthd_empty(buffer, offset, SNIP_EOSSNIP);
    return offset;
}

/*
 * @param[in] buffer
 * @param[in] offset
 * @return the new offset
 */
int recv_eos(uint8_t *buffer, int offset)
{
    int id, type, length;
    
    offset = parse(buffer, offset, &id, &type);
    if (id != SNIP_EOSSNIP || type != PROTOTYPE_LENGTHD)
        return -1;
    offset = parse_number(buffer, offset, &length);
    return offset;
}

/*
 * @param[in|out] buffer
 * @param[in] offset
 * @param[in] errorcode
 * @param[in] descr, string with error Description
 * @return the new offset
 */
int send_error(uint8_t *buffer, int offset, int errorcode , char *descr)
{
    long descr_length = strlen(descr);
    offset = add_type(buffer, offset, SNIPTYPE_ERROR);
    
    offset = serialize(buffer, offset, SNIP_ERRORSNIP, PROTOTYPE_LENGTHD);
    // calculate length of SNIP_ERRORCODE
    offset = serialize_number(buffer, offset, (int)descr_length+variant_length(ERRORSNIP_ERRORCODE, errorcode));
    
    offset = add_variant(buffer, offset, ERRORSNIP_ERRORCODE, errorcode);
    
    offset = add_lengthd(buffer, offset, ERRORSNIP_DESCRIPTION, (uint8_t*) descr, descr_length);
    
    return offset;
}

/*
 * @param[in] buffer
 * @param[in] offset
 * @param[out] errorcode
 * @param[out] descr, string with error Description , pointer to memory area of color [YOU have to FREE this Memory later!1!]
 * @return the new offset
 */
int recv_error(uint8_t *buffer, int offset, int *errorcode , char **descr)
{
    int id, type, length;
    offset = parse(buffer, offset, &id, &type); // Read first byte and check if Right snip
    if (id != SNIP_ERRORSNIP || type != PROTOTYPE_LENGTHD)
        return -1;
    
    offset = parse_number(buffer, offset, &length); // Read length of req_snip
    
    // Read Errorcode
    
    offset = parse(buffer, offset, &id, &type); // Read first byte and check if Right snip
    if (id != ERRORSNIP_ERRORCODE || type != PROTOTYPE_VARIANT)
        return -1;
    offset = parse_number(buffer, offset, errorcode); // Read value of SeqId

    
    // Read Error Description
    
    offset = parse(buffer, offset, &id, &type);
    if (id == ERRORSNIP_DESCRIPTION && type == PROTOTYPE_LENGTHD && offset > -1)
    {
        offset = parse_number(buffer, offset, &length);
        *descr = (char*) malloc((long) length+1);   // +1 for 0x00 (string end)
        memcpy(*descr, buffer+offset, (long) length);
        (*descr)[length] = 0x00; // string end
        offset += length;
    }
    else
    {
        return -1;
    }
    
    return offset;
}

/*
 * @param[out] buffer
 * @param[in] offset
 * @param[in] meta
 * @param[in] length_meta
 * @param[in] frame
 * @param[in] length_frame
 * @return the new offset
 */
int create_sequence(uint8_t *buffer, int offset, uint8_t *meta, long length_meta, uint8_t *frame, long length_frame)
{
    offset = add_lengthd(buffer, offset, BINARYSEQUENCE_METADATA, meta, length_meta);
    offset = add_lengthd(buffer, offset, BINARYSEQUENCE_FRAME, frame, length_frame);
    return offset;
}

/*
 * @param[in] buffer
 * @param[in] offset
 * @param[out] offset of metadata
 * @param[out] length of metadata
 * @param[out] offset of frame
 * @param[out] length of frame
 * @return the new offset
 */
int parse_sequence(uint8_t *buffer, int offset, int *meta_offset, int *length_meta, int *frame_offset, int *length_frame)
{
    int id, type;
    // Read Frames
    
    offset = parse(buffer, offset, &id, &type);
    if (id == BINARYSEQUENCE_METADATA && type == PROTOTYPE_LENGTHD && offset > -1)
    {
        offset = parse_number(buffer, offset, length_meta);
        (*meta_offset) = offset;
        offset += (*length_meta);
    }
    else
    {
        return -1;
    }
    // Read Frames
    
    offset = parse(buffer, offset, &id, &type);
    if (id == BINARYSEQUENCE_FRAME && type == PROTOTYPE_LENGTHD && offset > -1)
    {
        offset = parse_number(buffer, offset, length_frame);
        (*frame_offset) = offset;
        offset += (*length_frame);
    }
    else
    {
        return -1;
    }
    return offset;
}

/*
 * @param[out] buffer
 * @param[in] offset
 * @return the new offset
 */
int send_inforequest(uint8_t *buffer, int offset)
{
    offset = add_type(buffer, offset, SNIPTYPE_INFOREQUEST);
    offset = add_lengthd_empty(buffer, offset, SNIP_INFOREQUEST);
    return offset;
}

/*
 * @param[in] buffer
 * @param[in] offset
 * @return the new offset
 */
int recv_inforequest(uint8_t *buffer, int offset)
{
    int id, type, length;
    
    offset = parse(buffer, offset, &id, &type);
    if (id != SNIP_INFOREQUEST || type != PROTOTYPE_LENGTHD)
        return -1;
    offset = parse_number(buffer, offset, &length);
    return offset;
}

/*
 * @param[out] buffer
 * @param[in] offset
 * @param[in] meta, buffer with Binarysequenzemetadta
 * @return the new offset
 */
int send_infoanswer(uint8_t *buffer, int offset, uint8_t *meta, int length_meta)
{
    offset = add_type(buffer, offset, SNIPTYPE_INFOANSWER);
    
    offset = serialize(buffer, offset, SNIP_INFOANSWERSNIP, PROTOTYPE_LENGTHD);
    // calculate length of SNIP_Requestsnip
    offset = serialize_number(buffer, offset, length_meta);
    
    offset = add_lengthd(buffer, offset, INFOANSWERSNIP_META, meta, length_meta);
    
    return offset;
}

/*
 * @param[in] buffer
 * @param[in] offset
 * @param[out] offset of the metadata
 * @param[out] meta_length, length of the Metadata
 * @return the new offset
 */
int recv_infoanswer(uint8_t *buffer, int offset, int *meta_offset, int *meta_length)
{
    int id, type, length;
    offset = parse(buffer, offset, &id, &type); // Read first byte and check if Right snip
    if (id != SNIP_INFOANSWERSNIP || type != PROTOTYPE_LENGTHD)
        return -1;
    
    offset = parse_number(buffer, offset, &length); // Read length of req_snip
    
    // Read Metadata
    
    offset = parse(buffer, offset, &id, &type);  // Read Color
    if (id == INFOANSWERSNIP_META && type == PROTOTYPE_LENGTHD && offset > -1)
    {
        offset = parse_number(buffer, offset, meta_length);
        (*meta_offset) = offset;
        offset += (*meta_length);
    }
    else
    {
        return -1;
    }
    
    return offset;
}
