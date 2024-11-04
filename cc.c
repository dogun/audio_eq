#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int main(int argc, char *argv[]) {
	char* s_eq_file;
	double vol;
	if (argc < 2) {
		fprintf(stderr, "usage cc rewEqConfigFile \n");
		exit(1);
	}

	s_eq_file = argv[1];

	FILE* fp = fopen(s_eq_file, "r");
	if (fp == NULL) {
		fprintf(stderr, "open source config error:%s\n", s_eq_file);
		exit(1);
	}
	char line[200];
	while(!feof(fp)) {
		char* r = fgets(line, sizeof(line), fp);
		if (r == NULL || strlen(r) == 0) continue;
		if (!strstr(r, "Fc") || !strstr(r, "Gain") || !strstr(r, "Q")) continue;
		char* token = strtok(line, " ");
		double cf, gain, q;
		int i = 0;
		while (token != NULL) {
			//printf("--%s, %d\n", token, i);
			if (i == 5) cf = strtod(token, NULL);
			else if (i == 8) gain = strtod(token, NULL);
			else if (i == 11) q = strtod(token, NULL);
			i++;
			token = strtok(NULL, " ");
		}
		
		printf("%.0f %.1f %.3f\n", cf, gain, q);
		
	}
	fclose(fp);
}

