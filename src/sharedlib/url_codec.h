/*
 *  url_codec.h
 *  urlcodec
 *
 *  Created by Spiros Evangelatos on 30/05/2009.
 */

/*
 * Decode a URL by reversing the URL escaped sequences.
 * url: String holding the URL encoded data.
 * buff: Buffer to hold the decoded data.
 * buflen: Length of buff.
 * retain2f: Boolean flag. If set, the sequence %2F won't be decoded to '/'.
 * Returns 0 on success or -1 on failure.
 * It can fail if the length of buff is not sufficient, or an invalid 
 * escape sequence is encountered, such as %00 or %gh.
 */
int url_decode(const char *url, char *buff, int buflen, int retain2f);

/*
 * Encode a string by escaping all characters that are unsafe or ambiguous in a URL.
 * You can't pass a complete URL here. Instead you should pass fragments of a 
 * URL that you want to be safely encoded.
 * str: String to be encoded.
 * buff: Buffer to hold the encoded data.
 * buflen: Length of buff.
 * retain_slashes: Boolean flag. If set, '/' will not be escaped.
 * In some applications this can be ambiguous but is often OK and more aesthetically pleasant.
 * Returns 0 on success or -1 on failure.
 * It can fail if the length of buff is not sufficient.
 */
int url_encode(const char *str, char *buff, int buflen, int retain_slashes);