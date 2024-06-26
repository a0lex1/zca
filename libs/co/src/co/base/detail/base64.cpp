#include "./base64.h"
#include <memory.h>

namespace co {
namespace detail {

static const unsigned char base64_table[65] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

size_t base64_get_decoded_alloc_size(const void* _src, size_t encoded_size) {
	const unsigned char* src = (unsigned char*)_src;
	unsigned char dtable[256], *out, block[4], tmp;
	size_t i, count, olen;

	memset(dtable, 0x80, 256);
	for (i = 0; i < sizeof(base64_table) - 1; i++)
		dtable[base64_table[i]] = (unsigned char) i;
	dtable['='] = 0;

	count = 0;
	for (i = 0; i < encoded_size; i++) {
		if (dtable[src[i]] != 0x80)
			count++;
	}

	if (count == 0 || count % 4)
		return NULL;

	olen = count / 4 * 3;
	return olen;

}

size_t base64_get_encoded_alloc_size(size_t decoded_size, bool newlines) {
  size_t olen;
  olen = decoded_size * 4 / 3 + 4; /* 3-byte blocks to 4-byte */
	if (newlines) {
		olen += olen / 72; /* line feeds */
	}
	olen++; /* nul termination */
	return olen;
}

void base64_encode(
  const void *_src, size_t len,
  void* _out, size_t* out_len,
	bool newlines)
{
	const unsigned char* src = (unsigned char*)_src;
	unsigned char* out = (unsigned char*)_out;
	unsigned char *pos;
	const unsigned char *end, *in;
	int line_len;

	end = src + len;
	in = src;
	pos = out;
	line_len = 0;
	while (end - in >= 3) {
		*pos++ = base64_table[in[0] >> 2];
		*pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
		*pos++ = base64_table[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
		*pos++ = base64_table[in[2] & 0x3f];
		in += 3;
		line_len += 4;
		if (line_len >= 72) {
			if (newlines) {
				*pos++ = '\n';
			}
			line_len = 0;
		}
	}

	if (end - in) {
		*pos++ = base64_table[in[0] >> 2];
		if (end - in == 1) {
			*pos++ = base64_table[(in[0] & 0x03) << 4];
			*pos++ = '=';
		} else {
			*pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
			*pos++ = base64_table[(in[1] & 0x0f) << 2];
		}
		*pos++ = '=';
		line_len += 4;
	}

	if (newlines) {
		if (line_len)
			*pos++ = '\n';
	}

	//*pos = '\0';
	*out_len = pos - out;
}


bool base64_decode(const void* _src, size_t len,
  void* _out, size_t* out_len)
{
	const unsigned char* src = (unsigned char*)_src;
	unsigned char* out = (unsigned char*)_out;
	unsigned char dtable[256], *pos, block[4], tmp;
	size_t i, count, olen;
	int pad = 0;

	memset(dtable, 0x80, 256);
	for (i = 0; i < sizeof(base64_table) - 1; i++)
		dtable[base64_table[i]] = (unsigned char) i;
	dtable['='] = 0;

	count = 0;
	for (i = 0; i < len; i++) {
		if (dtable[src[i]] != 0x80)
			count++;
	}

	if (count == 0 || count % 4)
		return false;

	olen = count / 4 * 3;
	pos = out;
	if (out == NULL)
		return false;

	count = 0;
	for (i = 0; i < len; i++) {
		tmp = dtable[src[i]];
		if (tmp == 0x80)
			continue;

		if (src[i] == '=')
			pad++;
		block[count] = tmp;
		count++;
		if (count == 4) {
			*pos++ = (block[0] << 2) | (block[1] >> 4);
			*pos++ = (block[1] << 4) | (block[2] >> 2);
			*pos++ = (block[2] << 6) | block[3];
			count = 0;
			if (pad) {
				if (pad == 1)
					pos--;
				else if (pad == 2)
					pos -= 2;
				else {
					/* Invalid padding */
					return false;
				}
				break;
			}
		}
	}

	*out_len = pos - out;
	return out;
}


}}
