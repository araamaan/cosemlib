
#include <string.h>
#include "csm_ber.h"

// We only manage one-byte identifier
// We only manage two-bytes length


// 31 tags
static const char *cUniversalTypes[] = {
"Reserved",
"BOOLEAN",
"INTEGER",
"BIT STRING",
"OCTET STRING",
"NULL",
"OBJECT IDENTIFIER",
"ObjectDescriptor",
"INSTANCE OF",
"REAL",
"ENUMERATED",
"EMBEDDED PDV",
"UTF8String",
"RELATIVE-OID",
"SEQUENCE, SEQUENCE OF",
"SET, SET OF",
"NumericString",
"PrintableString",
"TeletexString, T61String",
"VideotexString",
"IA5String",
"UTCTime",
"GeneralizedTime",
"GraphicString",
"VisibleString, ISO646String",
"GeneralString",
"UniversalString",
"CHARACTER STRING",
"BMPString" };

static int csm_ber_read_tag(csm_array *i_array, ber_tag *o_tag)
{
    int ret = FALSE;
    uint8_t b;

    memset(o_tag, 0, sizeof(ber_tag));

    if (csm_array_read(i_array, &b))
    {
        o_tag->nbytes = 1;
        o_tag->tag = b;
        o_tag->cls = b & CLASS_MASK;
        o_tag->isPrimitive = (b & TYPE_MASK) == 0;
        o_tag->id = b & TAG_MASK;

        if (o_tag->id == TAG_MASK)
        {
            // Long tag, encoded as a sequence of 7-bit values, not supported
            CSM_ERR("[BER] Long tags are not supported");
        }
        else
        {
            ret = TRUE;
        }
    }
    return ret;
}

static int csm_ber_read_len(csm_array *i_array, ber_length *o_len)
{
    int ret = FALSE;
    uint8_t b;

    memset(o_len, 0, sizeof(ber_length));

    if (csm_array_read(i_array, &b))
    {
        o_len->nbytes = 1;
        o_len->length = b;

        if ((o_len->length & LEN_XTND) == LEN_XTND)
        {
            uint16_t numoct = o_len->length & LEN_MASK;

            o_len->length = 0;

            if (numoct > sizeof(o_len->length))
            {
                for (uint32_t i = 0; i < numoct; i++)
                {
                    if (csm_array_read(i_array, &b))
                    {
                        o_len->length = (o_len->length << 8U) | b;
                        o_len->nbytes++;
                        ret = TRUE;
                    }
                    else
                    {
                        ret = FALSE;
                        break;
                    }
                }
            }
        }
        else
        {
            ret = TRUE;
        }
    }
    return ret;
}


/**
 * @brief csm_ber_decode_object_identifier
 *
 *
    The first octet has value 40 * value1 + value2. (This is unambiguous, since
      --> value1 is limited to values 0, 1, and 2;
      --> value2 is limited to the range 0 to 39 when value1 is 0 or 1; and, according to X.208, n is always at least 2.)

    The following octets, if any, encode value3, ..., value n. Each value is encoded base 128,
    most significant digit first, with as few digits as possible, and the most significant
    bit of each octet except the last in the value's encoding set to "1."

    Example: The first octet of the BER encoding of RSA Data Security, Inc.'s object identifier
    is 40 * 1 + 2 = 42 = 2a16. The encoding of 840 = 6 * 128 + 4816 is 86 48 and the
    encoding of 113549 = 6 * 1282 + 7716 * 128 + d16 is 86 f7 0d.
    This leads to the following BER encoding:

    06 06 2a 86 48 86 f7 0d
 *
 * @return
 */

int csm_ber_decode_object_identifier(ber_object_identifier *oid, csm_array *array)
{
    int ret = FALSE;
    if (array->size == 7U)
    {
        const uint8_t header_size = sizeof(oid->header);
        // First copy the header
        csm_array header;
        csm_array_alloc(&header, oid->header, header_size);
        ret = csm_array_copy(&header, array);
        ret = ret && csm_array_jump(array, header_size);
        ret = ret && csm_array_read(array, &oid->name); // Then copy the object name
        ret = ret && csm_array_read(array, &oid->id); // Then copy the object id
    }
    return ret;
}

void csm_ber_dump(csm_ber *i_ber)
{
    CSM_TRACE("-------------- BER FIELD --------------\r\n");
    CSM_TRACE("Tag: ");

    if (i_ber->tag.cls == TAG_UNIVERSAL)
    {
        CSM_TRACE("Universal");
    }
    else if (i_ber->tag.cls == TAG_APPLICATION)
    {
        CSM_TRACE("Application");
    }
    else if (i_ber->tag.cls == TAG_CONTEXT_SPECIFIC)
    {
        CSM_TRACE("Context-specific");
    }
    else
    {
        CSM_TRACE("Private");
    }

    if (i_ber->tag.isPrimitive)
    {
        CSM_TRACE(" - Primitive");
    }
    else
    {
        CSM_TRACE(" - Constructed");
    }

    CSM_TRACE(" - %d(0x%02X)", i_ber->tag.tag, i_ber->tag.tag);

    if (i_ber->tag.isPrimitive && (i_ber->tag.cls == TAG_UNIVERSAL))
    {
        if (i_ber->tag.tag < 31U)
        {
            CSM_TRACE("%s", cUniversalTypes[i_ber->tag.tag]);
        }
        else
        {
            CSM_TRACE("Type: Unkown!");
        }
    }

    CSM_TRACE("\r\nValue length: %d\r\n", i_ber->length.length);
}

/*
// Private functions
static int csm_ber_is_universal(csm_ber *ber)
{
    return (ber->tag.cls == TAG_UNIVERSAL) ? TRUE : FALSE;
}

static int csm_ber_has_data(csm_ber *ber)
{
    int ret = FALSE;
    if ((ber->tag.cls == TAG_APPLICATION) || (ber->tag.cls == TAG_UNIVERSAL))
    {
        ret = TRUE;
    }
    return ret;
}
*/

int csm_ber_decode(csm_ber *ber, csm_array *array)
{
    int loop = TRUE;
    if (csm_ber_read_tag(array, &ber->tag))
    {
        if (csm_ber_read_len(array, &ber->length))
        {
            csm_ber_dump(ber);
            if (ber->tag.isPrimitive)
            {
                // This BER contains data, skip it and advance the pointer to the next BER header
                loop = csm_array_jump(array, ber->length.length);
            }
            // otherwise, loop again to decode next header
        }
        else
        {
            loop = FALSE;
        }
    }
    else
    {
        loop = FALSE;
    }

    return loop;
}