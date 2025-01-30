#include "cflbuild.h"

void
ReadPublicTxt(void)
{
	FILE *Fp;
	unsigned char FileBuf[128], ConfName[30];
	int Ret, CatNum = 0, ConfNum = 0, SerialNum = 0;

	Fp = fopen("/cyberspace/grex/bbs/public.txt", "r");

	if (Fp == NULL) {
		printf("File \"public.txt\" doesn't exist\n");
		FreeMemory();
		exit(0);
	}
	InitPublicConf();
	while (fgets(FileBuf, 128, Fp) != NULL) {
		Ret = ExtractConfName(FileBuf, ConfName);
		if (Ret == 1) {
			/* Category Line */

			WriteToPublicConf(FileBuf, ConfName, CatNum, ConfNum, SerialNum, 1);
			CatNum++;
			ConfNum = 0;
		} else if (Ret == 2) {
			/* Conf Line */

			WriteToPublicConf(FileBuf, ConfName, CatNum, ConfNum, SerialNum, 0);
			ConfNum++;
			SerialNum++;
		}
	}
	fclose(Fp);
}

void
InitPublicConf(void)
{
	CatList = AllocCats(5);
	CatList->Category[0] = 0x0;
	CatList->CatNum = -1;
	CatList->ConfPub = AllocConfs(5);
	CatList->ConfPub->ConfName[0] = 0x0;
	CatList->ConfPub->ConfNum = -1;
	CatList->ConfPub->SerialNum = -1;
	CatList->ConfPub->NextRecord = CatList->ConfPub->PrevRecord = NULL;
	CatList->NextRecord = CatList->PrevRecord = NULL;
}

struct Category *
AllocCats(int Len)
{
	Category *Cat;
	Cat = (struct Category *) malloc(sizeof(Category));
	if (Cat == NULL) {
		printf("Cannot Allocate Memory\n");
		FreeMemory();
		exit(0);
	}
	Cat->Category = (char *) malloc(Len);
	if (Cat->Category == NULL) {
		printf("Cannot Allocate Memory\n");
		FreeMemory();
		exit(0);
	}
	Cat->ConfPub = NULL;
	return Cat;
}
struct PublicConf *
AllocConfs(int Len)
{
	PublicConf *TempConf;

	TempConf = (struct PublicConf *) malloc(sizeof(PublicConf));
	if (TempConf == NULL) {
		printf("Cannot Allocate Memory\n");
		FreeMemory();
		exit(0);
	}
	TempConf->ConfName = (char *) malloc(Len);
	if (TempConf->ConfName == NULL) {
		printf("Cannot Allocate Memory\n");
		FreeMemory();
		exit(0);
	}
	return TempConf;
}

int
WriteToPublicConf(char *Buffer, char *ConfName, int CatNum, int ConfNum, int SerialNum, int WhichOne)
{
	Category *Cat;
	PublicConf *Pub;
	int Len;

	if (WhichOne) {
		/* CategoryLine */

		Len = strlen(ConfName);
		Cat = CatList;
		while (Cat->NextRecord != NULL)
			Cat = Cat->NextRecord;

		Cat->NextRecord = AllocCats(Len + 1);
		strlcpy(Cat->NextRecord->Category, ConfName, Len + 1);
		Cat->NextRecord->CatNum = CatNum;
		Cat->NextRecord->PrevRecord = Cat;
		Cat->NextRecord->NextRecord = NULL;
	} else {
		/* Conf Line */

		Len = strlen(Buffer);
		Cat = FindCat(CatNum - 1);
		if (Cat == NULL)
			return 0;
		if (Cat->ConfPub == NULL) {
			Cat->ConfPub = AllocConfs(Len + 1);
			strlcpy(Cat->ConfPub->ConfName, Buffer, Len + 1);
			Cat->ConfPub->ConfNum = ConfNum;
			Cat->ConfPub->SerialNum = SerialNum;
			Cat->ConfPub->PrevRecord = NULL;
			Cat->ConfPub->NextRecord = NULL;
		} else {
			Pub = Cat->ConfPub;
			while (Pub->NextRecord != NULL)
				Pub = Pub->NextRecord;

			Pub->NextRecord = AllocConfs(Len + 1);
			strlcpy(Pub->NextRecord->ConfName, Buffer, Len + 1);
			Pub->NextRecord->ConfNum = ConfNum;
			Pub->NextRecord->SerialNum = SerialNum;
			Pub->NextRecord->PrevRecord = Pub;
			Pub->NextRecord->NextRecord = NULL;
		}
	}
	return 1;

}

struct Category *
FindCat(int CatNum)
{

	Category *Cat;

	Cat = CatList->NextRecord;
	while (Cat != NULL) {
		if (Cat->CatNum == CatNum) {
			return Cat;
		}
		Cat = Cat->NextRecord;
	}
	return NULL;
}

void
DispCategories(void)
{
	Category *Cat;
	char Buffer[200];
	int Len;

	PrintCount = 0;
	printf("\nThese are the categories available :\n\n");
	Cat = CatList->NextRecord;
	while (Cat != NULL) {
		snprintf(Buffer, sizeof(Buffer), "%d.%s ", Cat->CatNum, Cat->Category);
		Len = strlen(Buffer);
		PrintString(Buffer, Len);
		Cat = Cat->NextRecord;
	}
	printf("\n\n");
}

void
DispAllPublicConfs(void)
{
	Category *Cat;
	PublicConf *Pub;
	int Ret, Len;
	char ConfName[200], Buffer[400];

	PrintCount = 0;
	printf("\nShort Listing Of All Conferences : \n\n");
	Cat = CatList->NextRecord;
	while (Cat != NULL) {
		Pub = Cat->ConfPub;
		while (Pub != NULL) {
			Ret = ExtractConfName(Pub->ConfName, ConfName);
			if (Ret == 2) {
				snprintf(Buffer, sizeof(Buffer), "%d.%s ", Pub->SerialNum, ConfName);
				Len = strlen(Buffer);
				PrintString(Buffer, Len);
			}
			Pub = Pub->NextRecord;
		}
		Cat = Cat->NextRecord;
	}
	printf("\n\n");
}

PublicConf *
DispPublicConf(int CatNum)
{
	Category *Cat;
	PublicConf *Pub;

	Cat = FindCat(CatNum);
	if (Cat == NULL)
		return NULL;

	printf("\nConferences availiable in \"%s\" :\n\n", Cat->Category);
	Pub = Cat->ConfPub;
	while (Pub != NULL) {
		printf("%d.%s", Pub->ConfNum, Pub->ConfName);
		Pub = Pub->NextRecord;
	}
	printf("\n\n");
	return Cat->ConfPub;
}

int
SelectList(void)
{
	char Str[10];
	int Choice;

	do {
		printf("Select from below....\n");
		printf("1. Short Listing Of Conferences\n");
		printf("2. Long  Listing Of Conferences\n");
		printf("3. Quit \n\n");
		printf("Enter your choice : ");
		GetString(Str, 5);

		Choice = atoi(Str);

	} while (Choice < 1 || Choice > 3);

	return Choice;
}

void
FreePublicConf(void)
{
	Category *Cat, *TempCat;
	PublicConf *Pub, *TempPub;

	Cat = CatList;
	while (Cat != NULL) {
		TempCat = Cat->NextRecord;
		free(Cat->Category);
		Pub = Cat->ConfPub;
		while (Pub != NULL) {
			TempPub = Pub->NextRecord;
			free(Pub->ConfName);
			free(Pub);
			Pub = TempPub;
		}
		free(Cat);
		Cat = TempCat;
	}
	CatList = NULL;
}

int
ExtractConfName(char *FileBuf, char *ConfName)
{
	/* Returns 0 if blank line or non category line */
	/* Returns 1 if its category line */
	/* Returns 2 if conf name line */

	int i, j, k = 0;

	if (FileBuf[0] != 0x20) {
		if (strchr(FileBuf, (int) ':') != NULL)
			k = 1;
		else
			return 0;
	}
	if (Isblank(FileBuf))
		return 0;
	i = 0;
	while (FileBuf[i] == 0x20 || FileBuf[i] == '\t')
		i++;
	j = 0;
	if (k) {
		while (FileBuf[i] != ':' && FileBuf[i] != '\n' && FileBuf[i] != 0x0) {
			ConfName[j++] = FileBuf[i++];
		}
	} else {
		while (FileBuf[i] != 0x20 && FileBuf[i] != '\t') {
			ConfName[j++] = FileBuf[i++];
		}
	}
	ConfName[j] = 0x0;
	if (k)
		return 1;
	else
		return 2;
}

int
Isblank(char *FileBuf)
{
	int i, Len;

	Len = strlen(FileBuf);
	for (i = 0; i < Len; i++) {
		if (FileBuf[i] == 0x0d || FileBuf[i] == 0x0a ||
		    FileBuf[i] == 0x0 || FileBuf[i] == 0x9)
			continue;
		if (FileBuf[i] != 0x20)
			return 0;
	}
	return 1;
}
int
GetConfFromPublicTxt(PublicConf * Pub, int ConfNum, char *ConfName)
{
	Category *Cat;
	PublicConf *TempPub;
	int Ret;

	if (Pub == NULL) {
		Cat = CatList->NextRecord;
		while (Cat != NULL) {
			TempPub = Cat->ConfPub;
			while (TempPub != NULL) {
				if (TempPub->SerialNum == ConfNum) {
					Ret = ExtractConfName(TempPub->ConfName, ConfName);
					return Ret;
				}
				TempPub = TempPub->NextRecord;
			}
			Cat = Cat->NextRecord;
		}
	} else {
		while (Pub != NULL) {
			if (Pub->ConfNum == ConfNum) {
				Ret = ExtractConfName(Pub->ConfName, ConfName);
				return Ret;
			}
			Pub = Pub->NextRecord;
		}
	}
	printf("The conference number \"%d\" is not found\n", ConfNum);
	return 0;
}

int
Move(void)
{
	ConfList *Conf1, *Conf2;
	int Node1, Node2;
	char Str[10];

	DisplayList();
	printf("Enter the conf num to be moved or 'q' to quit: ");
	GetString(Str, 5);
	if (Str[0] == 'q')
		return 0;
	if (Isblank(Str)) {
		printf("Conf num not entered quitting..\n");
		return 0;
	}
	Node1 = atoi(Str);
	printf("Enter the position where it should be moved or 'q' to quit: ");
	GetString(Str, 5);
	if (Str[0] == 'q')
		return 0;
	if (Isblank(Str)) {
		printf("Conf num not entered quitting..\n");
		return 0;
	}
	Node2 = atoi(Str);
	if (Node1 == Node2) {
		printf("\nNumbers cannot be the same\n");
		return 0;
	}
	Conf1 = Find(Node1, "Dummy", 0);
	if (Conf1 != NULL) {
		Conf2 = Find(Node2, "Dummy", 0);
		if (Conf2 != NULL) {
			/* Delete Conf1 and insert it before Conf2 */
			Insert(Conf1, Conf2);
		} else {
			printf("Conf with num \"%d\" not Found\n", Node2);
			return 0;
		}
	} else {
		printf("Conf with num \"%d\" not Found\n", Node1);
		return 0;
	}
	printf("\nConference %d : %s moved before %d : %s\n",
	       Conf1->ConfNum, Conf1->ConfName,
	       Conf2->ConfNum, Conf2->ConfName);
	Shuffle();
	WriteTocflist();
	return 1;

}

void
Insert(ConfList * Conf1, ConfList * Conf2)
{
	Conf1->PrevRecord->NextRecord = Conf1->NextRecord;
	if (Conf1->NextRecord != NULL)
		Conf1->NextRecord->PrevRecord = Conf1->PrevRecord;
	Conf1->PrevRecord = Conf2->PrevRecord;
	if (Conf2->PrevRecord != NULL)
		Conf2->PrevRecord->NextRecord = Conf1;
	Conf2->PrevRecord = Conf1;
	Conf1->NextRecord = Conf2;
}

int
LongList(void)
{
	PublicConf *Pub;
	char Str[10], Yes;
	int CatNum;

	do {
		DispCategories();
		printf("Enter the no to display all confs\n");
		printf("in that category Or 'q' to quit : ");
		GetString(Str, 5);
		if (Str[0] == 'q')
			return 0;
		if (Isblank(Str)) {
			printf("\nCategory num not entered Quitting...\n");
			return 0;
		}
		CatNum = atoi(Str);
		Pub = DispPublicConf(CatNum);
		if (Pub == NULL) {
			printf("Category \"%d\" doesn't exist\n", CatNum);
			return 0;
		}
		printf("List categories again press (y/n): ");
		Yes = getchar();
		getc(stdin);
		if (Yes != 'y')
			break;

	} while (0x9);
	return 1;

}

void
GetString(char *Str, int Len)
{
	/* This program doesn't include null in Len chars */

	int i = 0;
	char Ch;

	do {
		Ch = getchar();
		if (Ch < 0) {
			FreeMemory();
			printf("\n\nCtrl Character Encountered...\n");
			printf("Exiting the program...\n");
			printf("\nThanks for using .cflist builder program \"%s\". Bye!!!\n\n", User);
			exit(0);
		}
		if (Ch == 0x0d || Ch == 0x0a || i >= Len) {
			Str[i] = 0x0;
			break;
		} else {
			if ((Ch >= '0' && Ch <= '9') || Ch == 0x20 || Ch == 'q')
				Str[i++] = Ch;
		}

	} while (0x9);
	/* Flush keyboard buffer */
	fflush(stdin);
}

void
PrintString(char *Buffer, int Len)
{
	int i;

	for (i = 0; i < Len; i++) {
		if (PrintCount >= 78) {
			PrintCount = 0;
			printf("\n");
		}
		printf("%c", Buffer[i]);
		PrintCount++;
	}
}
