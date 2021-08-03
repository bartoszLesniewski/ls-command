#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

#define MAX_STRING_LENGTH 512

// typ wyliczeniowy określający format, w jakim może być wypisywany właściciel oraz grupa
enum format
{
	NAME = 0,
	NUMBER = 1,
	NAME_NO_GROUP = 2,
	NUMBER_NO_GROUP = 3
};

// wypisuje typ pliku
void print_file_type(unsigned int type)
{
	switch (type)
	{
		case DT_BLK:
			printf("b");
			break;
		case DT_CHR:
			printf("c");
			break;
		case DT_DIR:
			printf("d");
			break;
		case DT_FIFO:
			printf("p");
			break;
		case DT_LNK:
			printf("l");
			break;
		case DT_REG:
			printf("-");
			break;
		case DT_SOCK:
			printf("s");
			break;
	}
}

// wypisuje prawa dostępu do pliku
void print_permissions(mode_t stat_mode)
{
	// prawa użytkownika
	if (stat_mode & S_IRUSR)
		printf("r");
	else
		printf("-");

	if (stat_mode & S_IWUSR)
		printf("w");
	else
		printf("-");

	if (stat_mode & S_IXUSR)
		printf("x");
	else
		printf("-");

	// prawa grupy
	if (stat_mode & S_IRGRP)
		printf("r");
	else
		printf("-");

	if (stat_mode & S_IWGRP)
		printf("w");
	else
		printf("-");

	if (stat_mode & S_IXGRP)
		printf("x");
	else
		printf("-");

	// prawa pozostałych
	if (stat_mode & S_IROTH)
		printf("r");
	else
		printf("-");

	if (stat_mode & S_IWOTH)
		printf("w");
	else
		printf("-");

	if (stat_mode & S_IXOTH)
		printf("x");
	else
		printf("-");
}

// wypisuje właściciela oraz grupę w podanym formacie
void print_owner_and_group(uid_t user_id, gid_t group_id, enum format f)
{
	struct passwd *user_info = getpwuid(user_id);
	struct group *group_info = getgrgid(group_id);

	if (f == NAME)
		printf("%s %s ", user_info->pw_name, group_info->gr_name);
	else if (f == NUMBER)
		printf("%lu %lu ", (unsigned long)user_id, (unsigned long)group_id);
	else if (f == NAME_NO_GROUP)
		printf("%s ", user_info->pw_name);
	else
		printf("%lu ", (unsigned long)user_id); 
}

// wypisuje czas ostatniej modyfikacji
void print_time_of_last_modification(time_t time)
{
	char date[80];
	strftime(date, 80, "%b %d %H:%M ", localtime(&time));
	printf("%s", date);
}

// główna funkcja wypisująca szczegółowe informacje o pliku (opcja -l)
void print_with_details(struct dirent *pDirEnt, struct stat statistics, enum format f)
{
	print_file_type(pDirEnt->d_type);
	print_permissions(statistics.st_mode);
	printf(" %u ", (unsigned int)statistics.st_nlink);
	print_owner_and_group(statistics.st_uid, statistics.st_gid, f);
	printf("%llu ", (unsigned long long)statistics.st_size);
	print_time_of_last_modification(statistics.st_mtime);
}

// zwraca 1, jeśli znaleziono opcję, 0 w przeciwnym razie
int check_option(char opt, int argc, char *argv[])
{
	// zaczyna od 1, bo pod 0 jest nazwa pliku
	for (int i = 1; i < argc; i++)
	{
		for (int j = 0; j < strlen(argv[i]); j++)
		{
			if (argv[i][0] == '-' && argv[i][j] == opt)
				return 1;
		}
	}

	return 0;
}

// sprawdza, które z dostępnych opcji zostały podane jako parametry i wywołuje dla nich odpowiednie funkcje
void check_all_options(struct dirent *pDirEnt, struct stat statistics, int argc, char *argv[])
{
	if (check_option('i', argc, argv))
		printf("%lu ", (unsigned long)statistics.st_ino);
	if (check_option('n', argc, argv) && check_option('G', argc, argv))
		print_with_details(pDirEnt, statistics, NUMBER_NO_GROUP);
	else if (check_option('n', argc, argv))
		print_with_details(pDirEnt, statistics, NUMBER);
	else if (check_option('l', argc, argv) && check_option('G', argc, argv))
		print_with_details(pDirEnt, statistics, NAME_NO_GROUP);
	else if (check_option('l', argc, argv))
		print_with_details(pDirEnt, statistics, NAME);
}

void list(char *path, int argc, char *argv[]);

// wyszukuje podkatalogi i wywołuje dla nich funkcję listującą (opcja -R)
void list_directories(char *path, int argc, char *argv[])
{
	DIR *pDIR;
	pDIR = opendir(path);
	struct dirent *pDirEnt = readdir(pDIR);;
	char file_location[MAX_STRING_LENGTH];

	printf("\n");

	while (pDirEnt != NULL)
	{
		sprintf(file_location, "%s%s%s", path, "/", pDirEnt->d_name);

		if ((pDirEnt->d_type == DT_DIR) && (check_option('a', argc, argv) || pDirEnt->d_name[0] != '.') && (strcmp(pDirEnt->d_name, ".") != 0 && strcmp(pDirEnt->d_name, "..") != 0))
			list(file_location, argc, argv);

		pDirEnt = readdir(pDIR);
	}

}

// główna funkcja listująca zawartość katalogu
void list(char *path, int argc, char *argv[])
{
	DIR *pDIR;
	struct dirent *pDirEnt;
	struct stat statistics;
	pDIR = opendir(path);

	if (pDIR == NULL) 
	{
		fprintf(stderr, "%s %d: opendir() failed (%s)\n", __FILE__, __LINE__, strerror(errno));
		exit(-1);
	}

	pDirEnt = readdir(pDIR);
	char file_location[MAX_STRING_LENGTH];

	if (check_option('R', argc, argv))
		printf("%s:\n", path);

	while (pDirEnt != NULL) 
	{
		sprintf(file_location, "%s%s%s", path, "/", pDirEnt->d_name);
		stat(file_location, &statistics);

		if ((check_option('a', argc, argv)) || (strcmp(pDirEnt->d_name, ".") != 0 && strcmp(pDirEnt->d_name, "..") != 0 && pDirEnt->d_name[0] != '.'))
		{

			check_all_options(pDirEnt, statistics, argc, argv);
			printf("%s\n", pDirEnt->d_name);
		}

		pDirEnt = readdir(pDIR);
	}

	if (check_option('R', argc, argv))
		list_directories(path, argc, argv);
	
	closedir(pDIR);
}

// zwraca ścieżkę do katalogu do wylistowania podanego jako parametr (jeśli nie podano parametru, to jest to bieżący katalog - ".")
char* get_path(int argc, char *argv[])
{
	if (argc - 1 > 0 && argv[argc - 1][0] != '-')
		return argv[argc - 1];

	return ".";
}

int main(int argc, char *argv[]) 
{
	char *path = get_path(argc, argv);
	list(path, argc, argv);

	return 0;
}
