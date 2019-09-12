#define _CRT_SECURE_NO_WARNINGS

#include "scan_dir.h"
#include "dirent.h"


char current_file[MAX_FILES][MAX_NAME_LENGTH];


inline const char* concat(const char* s1, const char* s2)
{
	char* result = malloc(strlen(s1) + strlen(s2) + 1);
	strcpy(result, s1);
	strcat(result, s2);

	return result != 0 ? result : NULL;
}


int scan_dir(char* dir_path)
{
	DIR* dir;
	struct dirent* ent;
	int i = -2;

	if ((dir = opendir(dir_path)) != 0)
	{
		while ((ent = readdir(dir)) != 0)
		{
			if (++i > 0)
			{
				printf("\n\t %u. %s", i, ent->d_name);
				strcpy(current_file[i], ent->d_name);
			}
		}
		free(ent);
		closedir(dir);
	}
	else
	{	
		return -1;
	}

	//printf("\r\n");
	return i;
}
