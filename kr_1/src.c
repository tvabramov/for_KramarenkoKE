#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NAME_LEN 15
#define MAX_FNAME_LEN 20

struct PhoneBookRec {
	char name[MAX_NAME_LEN + 1];
	char fname[MAX_FNAME_LEN + 1];
	int phone;
	struct PhoneBookRec *next;
};

void init(struct PhoneBookRec **pb);
void printHelp();
void printPhoneBookHeader();
void printPhoneBook(struct PhoneBookRec *pb);
void addRecord(struct PhoneBookRec **pb, char* name, char* fname, int phone);
int delRecord(struct PhoneBookRec **pb, char* name, char* fname, int phone);
void removePhoneBook(struct PhoneBookRec **pb);
struct PhoneBookRec *findRecords(struct PhoneBookRec *pb, char* name, char* fname, int phone);

int main(int argc, char **argv)
{
	struct PhoneBookRec *pb = NULL;

	init(&pb);

	printf("Phone Book Database. Type \"help\" to get commands list or \"exit\" to exit\n");
	for ( ; ; )
	{
		char command[1024];
		memset(command, 0, sizeof(char) * 1024);

		printf("> ");
		fgets(command, 1023, stdin);

		// Trim the string
		while ((strlen(command) > 0) && ((command[strlen(command) - 1] == '\r') || (command[strlen(command) - 1] == '\n')))
			command[strlen(command) - 1] = '\0';

		if (strcmp(command, "help") == 0) {
			printHelp();
			continue;
		}

		if (strcmp(command, "exit") == 0) {
			break;
		}

		if (strcmp(command, "list") == 0) {
			printPhoneBook(pb);
			continue;
		}

		if (strstr(command, "findname ") == command) {
			char buf[1024];
			char name[1024];
			sscanf(command, "%s %s", buf, name);

			struct PhoneBookRec *newpb = findRecords(pb, name, NULL, -1);

			printPhoneBook(newpb);//printf("name = %s\n", name);

			removePhoneBook(&pb);
			continue;		
		}

		if (strstr(command, "findfname ") == command) {
                        char buf[1024];
                        char fname[1024];
                        sscanf(command, "%s %s", buf, fname);

                        struct PhoneBookRec *newpb = findRecords(pb, NULL, fname, -1);

                        printPhoneBook(newpb);//printf("name = %s\n", name);

                        removePhoneBook(&pb);
                        continue;
                }

		if (strstr(command, "findphone ") == command) {
                        char buf[1024];
			int phone;
			sscanf(command, "%s %d", buf, &phone);

                        struct PhoneBookRec *newpb = findRecords(pb, NULL, NULL, phone);

                        printPhoneBook(newpb);//printf("name = %s\n", name);

                        removePhoneBook(&pb);
                        continue;
                }

		if (strstr(command, "add ") == command) {
			char buf[1024];
			char name[1024];
			char fname[1024];
			int phone;

			sscanf(command, "%s %s %s %d", buf, name, fname, &phone);

			addRecord(&pb, name, fname, phone);
			continue;
		}

		if (strstr(command, "delname ") == command) {
			char buf[1024];
			char name[1024];
                        sscanf(command, "%s %s", buf, name);

			int delcnt = delRecord(&pb, name, NULL, -1);

			printf("Removed %d records\n", delcnt);
			continue;
                }

		if (strstr(command, "delfname ") == command) {
                        char buf[1024];
                        char fname[1024];
                        sscanf(command, "%s %s", buf, fname);

                        int delcnt = delRecord(&pb, NULL, fname, -1);

			printf("Removed %d records\n", delcnt);
                        continue;
                }

		if (strstr(command, "delphone ") == command) {
                        char buf[1024];
			int phone;
                        sscanf(command, "%s %d", buf, &phone);

			int delcnt = delRecord(&pb, NULL, NULL, phone);

			printf("Removed %d records\n", delcnt);
                        continue;
                }

		fprintf(stderr, "Error: unrecognized command\n");
	}

	return 0;
}

void init(struct PhoneBookRec **pb)
{
	*pb = NULL;

	addRecord(pb, "Bidon", "Pomoev", 12345);
	addRecord(pb, "Vedron", "Pomoev", 12346);
	addRecord(pb, "Rulon", "Oboev", 23456);
	addRecord(pb, "Chered", "Zastoev", 34567);
	addRecord(pb, "Ushat", "Pomoev", 45678);
	addRecord(pb, "Ashraf", "Sobhi_Abdel-Maskud_Abdel-Mottaleb", 56789);
}

void printHelp()
{
	printf("\thelp                        - get help\n");
	printf("\tlist                        - print all records\n");
	printf("\tadd <name> <fname> <phone>  - add the new abonent\n");
	printf("\tdelname <name>              - delete by name\n");
	printf("\tdelfname <fname>            - delete by family name\n");
	printf("\tdelphone <fname>            - delete by phone\n");
	printf("\tfindname <name>             - find by name\n");
	printf("\tfindfname <fname>           - find by family name\n");
	printf("\tfindphone <phone>           - find by phone\n");
	printf("\texit                        - exit\n");
}

void printPhoneBookHeader()
{
	printf("\tN\t\tName\t\t\tFamily_Name\t\t\tPhone\n");
        printf("\t-\t\t----\t\t\t-----------\t\t\t-----\n");
}

void printPhoneBook(struct PhoneBookRec *pb)
{
	printPhoneBookHeader();

	struct PhoneBookRec *curr = pb;
	int i = 1;
	while (pb != NULL) {
		printf("\t%d\t\t%s\t\t\t%s\t\t\t\t%d\n", i, pb->name, pb->fname, pb->phone);
		pb = pb->next;
		i++;
	}
}

void addRecord(struct PhoneBookRec **pb, char* name, char* fname, int phone)
{
	struct PhoneBookRec *newrec = (struct PhoneBookRec *)malloc(sizeof(struct PhoneBookRec));

	memset(newrec->name, 0, sizeof(char) * (MAX_NAME_LEN + 1));
	memcpy(newrec->name, name, (strlen(name) > MAX_NAME_LEN) ? MAX_NAME_LEN : strlen(name));

	memset(newrec->fname, 0, sizeof(char) * (MAX_FNAME_LEN + 1));
        memcpy(newrec->fname, fname, (strlen(fname) > MAX_FNAME_LEN) ? MAX_FNAME_LEN : strlen(fname));

	newrec->phone = phone;

	if ((*pb) == NULL) (*pb) = newrec;
	else {
		struct PhoneBookRec *last = *pb;
		while (last->next != NULL) last = last->next;
		last->next = newrec;
	}
}

// Checking, if rec->name == name or NULL, rec->fname == fname or NULL, rec->phone == phone or -1
// 0 if matches
int RecordMathces(struct PhoneBookRec *rec, char* name, char* fname, int phone)
{
	if (((name == NULL) || (strcmp(rec->name, name) == 0)) &&
	    ((fname == NULL) || (strcmp(rec->fname, fname) == 0)) &&
            ((phone < 0) || rec->phone == phone)) return 0;
	else return 1;
}

// retval - amount of deleted
int delRecord(struct PhoneBookRec **pb, char* name, char* fname, int phone)
{
	if ((*pb) == NULL) return 0;
	else if ((*pb)->next == NULL) {
		if (RecordMathces(*pb, name, fname, phone) == 0) {
			free(*pb);
			*pb = NULL;
			return 1;
		}
		else return 0;
	}
	else {
		int cnt = 0;

		while (((*pb) != NULL) && (RecordMathces(*pb, name, fname, phone) == 0)) {
			struct PhoneBookRec *buf = *pb;
			*pb = (*pb)->next;
			free(buf);
			cnt++;
		}

		struct PhoneBookRec *curr = *pb;
		
		while (curr->next != NULL)
		{ 
			if (RecordMathces(curr->next, name, fname, phone) == 0) {
				struct PhoneBookRec *buf = curr->next;
				curr->next = buf->next;
				free(buf);
				cnt++;
			}
			else curr = curr->next;
		}

		return cnt;
	}
}

void removePhoneBook(struct PhoneBookRec **pb)
{
	struct PhoneBookRec *curr = *pb;

	while (curr != NULL) {
		struct PhoneBookRec *buf = curr;
		curr = curr->next;
		free(buf);
	}

	*pb = NULL;
}

// Returns new list, that contains mathcing records
struct PhoneBookRec *findRecords(struct PhoneBookRec *pb, char* name, char* fname, int phone)
{
	struct PhoneBookRec *res = NULL;

	struct PhoneBookRec *curr = pb;
	while (curr != NULL) {
		if (RecordMathces(curr, name, fname, phone) == 0)
			addRecord(&res, curr->name, curr->fname, curr->phone);

		curr = curr->next;
	}

	return res;
}
