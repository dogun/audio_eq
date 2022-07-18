/*
 * util.h
 *
 *  Created on: 2022Äê7ÔÂ14ÈÕ
 *      Author: yuexu
 */

#ifndef MAIN_CONFIG_UTIL_H_
#define MAIN_CONFIG_UTIL_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

char* _new_str(char* o_str) {
	if (o_str == NULL) return o_str;
	char* ret = (char*)calloc(strlen(o_str) + 1, sizeof(char));
	strcpy(ret, o_str);
	return ret;
}

int _strsep_cnt(char* str, const char* sep) {
	if (NULL == str || NULL == sep) return 0;
	char* n_str = _new_str(str);
	char* tk;
	char* bbf = n_str;
	int count = 0;
	while ((tk = strsep(&bbf, sep)) != NULL) {
		if (strlen(tk) == 0) continue;
		count++;
	}
	free(n_str);
	return count;
}

#endif /* MAIN_CONFIG_UTIL_H_ */
