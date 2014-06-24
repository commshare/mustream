/*
 *  url_codec.c
 *  urlcodec
 *
 *  Created by Spiros Evangelatos on 30/05/2009.
 */

#include "url_codec.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <inttypes.h>	/* strtol */
#include <string.h>

/*
 * returns <>0 if c is a valid hexadecimal digit, else 0 
 */
static int ishexdigit(char c)
{
	return isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

/*
 * Returns the integer value represented by a two
 * digit hexadecimal string (0-255) or -1 on error.
 */
static int unhex(const char hex[])
{
	int val;
	char str[3], *endptr = str;
	/* make sure it's hex digits */
	if (!ishexdigit(hex[0]) || !ishexdigit(hex[1]))
		return -1;
	str[0] = hex[0];
	str[1] = hex[1];
	str[2] = '\0';
	val = (int) strtol(str, &endptr, 16);
	if (*endptr != '\0')
		return -1;
	return val;
}

/*
 * Returns the integer value represented by a two
 * digit hexadecimal string (0-255) or -1 on error.
 */
static int in_set(char c, char *set)
{
	while (*set != '\0') {
		if (c == *set)
			return 1;
		set++;
	}
	return 0;
}

/* 
 * Append a single character on the string in buff 
 */
static int append_char(char *buff, char c, int buflen)
{
	int i;
	for(i=0; i+1 < buflen; i++)
		if (buff[i] == '\0') {
			buff[i] = c;
			buff[i+1] = '\0';
			return 0;
		}
	/* out of space in the buffer */
	return -1;
}

int url_decode(const char *url, char *buffer, int buflen, int retain2f)
{
	int i = 0, j = 0, c;
	for(i=0; (c = url[i]) != '\0'; i++) {		
		/* reverse escaped chars */
		if(c == '%') {
			if((c = unhex(&url[i+1])) < 0)
				goto err;
			/* don't permit \0 since it would terminate the string early */
			if (c == '\0')
				goto err;
			
			/* possibly retain %2F */
			if (retain2f && c == 0x2F)
				c = url[i];
			else
				i += 2;
		}
		
		/* don't write past end of buffer */
		if (j+1 >= buflen)
			goto err;
		/* copy character to output buffer */
		buffer[j++] = (char) c;
	}
	
	buffer[j] = '\0';
	/* success */
	return 0;
err:
	buffer[0] = '\0';
	return -1;
}


int url_encode(const char *str, char *buff, int buflen, int retain_slashes)
{
	int i, c;
	char *retained, hexbuf[4];
	
	if (retain_slashes)
		retained = "-_.!~*'()/";
	else
		retained = "-_.!~*'()"; /* according to RFC 2396, these are safe */
	
	/* empty buffer */
	buff[0] = '\0';
	for (i = 0; (c = str[i]) != '\0'; i++) {
		/* just copy the retained and alphanumeric */
		if (isalnum(c) || in_set(c, retained)) {
			if (append_char(buff, c, buflen) < 0)
				return -1;
			buflen--;
			buff++; /* move the buff, to speed up appends */
			continue;
		}
		
		/* everything else has to be escaped */
		snprintf(hexbuf, 4, "%%%02X", (unsigned char)c);
		if (buflen < 4)
			return -1;
		strncat(buff, hexbuf, buflen);
		buflen -= 3; /* we wrote "%20", that is 3 characters */
		buff += 3;
	}
	
	return 0;
}
