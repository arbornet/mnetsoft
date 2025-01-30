#include "cflbuild.h"

Category *CatList;
ConfList *Conf;
char CflName[200], User[128], Home[1024];
int PrintCount;
struct passwd *PassWord;

int
main(void)
{
	FILE *Fp;
	char FileBuf[1024], ConfBuf[200], Yes;
	int Choice, Num = 0, Len, Index = 0;

	signal(SIGINT, Ctrl_C_Handler);

	printf("\n\".cflist\" builder program version 1.0 written by \"nt\"\n");

	PassWord = getpwuid(getuid());
	strlcpy(User, PassWord->pw_name, sizeof(User));
	strlcpy(Home, PassWord->pw_dir, sizeof(Home));

	/* .cflist is opened if it already exists or it will ask */
	/* the user to create one. User has the choice to create or not */

	snprintf(CflName, sizeof(CflName), "%s/.cfdir/.cflist", Home);

	if ((Fp = fopen(CflName, "r+")) == NULL) {
		snprintf(CflName, sizeof(CflName), "%s/.cflist", Home);

		if ((Fp = fopen(CflName, "r+")) == NULL) {
			printf("\nFile \"%s\" doesn't exist\n", CflName);
			printf("Create \".cflist\" press(y/n) : ");
			scanf("%c", &Yes);
			/* Store Newline */
			getc(stdin);
			if (Yes != 'y') {
				printf("\n\"%s\" chosen \"%c\" to create \"%s\"\n", User, Yes, CflName);
				exit(0);
			}
			if ((Fp = fopen(CflName, "w+")) == NULL) {
				printf("Cannot create file \"%s\"\n", CflName);
				exit(0);
			}
		}
	}
	PrintCount = 0;
	InitList();
	while (fgets(FileBuf, 128, Fp) != NULL) {
		if (!Isblank(FileBuf)) {
			/* Removing CR & LF at the end of FileBuf */
			Len = strlen(FileBuf);
			FileBuf[--Len] = 0x0;
			while (Index < Len) {
				ExtractTokens(FileBuf, &Index, ConfBuf);
				WriteToBuffers(ConfBuf, Num, Len);
				Num++;
			}
			Index = 0;
		}
	}
	fclose(Fp);
	ReadPublicTxt();
	do {
		Choice = DispMenu();
		switch (Choice) {
		case 1:
			Add();
			break;

		case 2:
			Change();
			break;

		case 3:
			Delete();
			break;

		case 4:
			List();
			break;

		case 5:
			Sort();
			break;

		case 6:
			putchar(0x7);
			break;

		default:
			break;
		}

	} while (Choice != 6);
	/* Free all memory b4 exiting the program */
	FreeMemory();
	printf("\nThanks for using .cflist builder program \"%s\". Bye!!!\n\n", User);
	exit(0);

}

void
InitList(void)
{
	Conf = (struct ConfList *) malloc(sizeof(ConfList));
	if (Conf == NULL) {
		printf("Cannot Allocate Memory\n");
		FreeMemory();
		exit(0);
	}
	Conf->ConfName = (char *) malloc(10);
	if (Conf->ConfName == NULL) {
		printf("Cannot Allocate Memory\n");
		FreeMemory();
		exit(0);
	}
	Conf->ConfName[0] = 0x0;
	Conf->NextRecord = Conf->PrevRecord = NULL;
}

void
WriteToBuffers(unsigned char *Buffer, int Num, int Len)
{
	ConfList *TempConf;

	TempConf = Conf;
	while (TempConf->NextRecord != NULL) {
		TempConf = TempConf->NextRecord;
	}
	TempConf->NextRecord = (struct ConfList *) malloc(sizeof(ConfList));
	if (TempConf->NextRecord == NULL) {
		printf("Cannot Allocate Memory\n");
		FreeMemory();
		exit(0);
	}
	TempConf->NextRecord->ConfName = (char *) malloc(Len + 1);
	if (TempConf->NextRecord->ConfName == NULL) {
		printf("Cannot Allocate Memory\n");
		FreeMemory();
		exit(0);
	}
	strlcpy(TempConf->NextRecord->ConfName, Buffer, Len + 1);
	TempConf->NextRecord->ConfNum = Num;
	TempConf->NextRecord->PrevRecord = TempConf;
	TempConf->NextRecord->NextRecord = NULL;

}

int
DisplayList(void)
{
	ConfList *TempConf;
	char Buffer[200];
	int Len;

	if (Conf == NULL)
		return 0;
	PrintCount = 0;
	printf("\nConferences availiable in your \".cflist\" :\n\n");
	TempConf = Conf->NextRecord;
	while (TempConf != NULL) {
		snprintf(Buffer, sizeof(Buffer), "%d.%s ", TempConf->ConfNum, TempConf->ConfName);
		Len = strlen(Buffer);
		PrintString(Buffer, Len);
		TempConf = TempConf->NextRecord;
	}
	printf("\n\n");
	return 1;
}

void
FreeList(void)
{
	ConfList *TempConf;

	TempConf = Conf;
	while (Conf != NULL) {
		TempConf = Conf->NextRecord;
		free(Conf->ConfName);
		free(Conf);
		Conf = TempConf;
	}
}

int
Delete(void)
{
	ConfList *TempConf;
	char ConStr[10];
	int ConfNum;

	DisplayList();
	printf("Enter the conference number to delete or 'q' to quit: ");
	GetString(ConStr, 5);
	if (ConStr[0] == 'q')
		return 0;
	if (Isblank(ConStr)) {
		printf("Cannot delete the conf no : %s\n", ConStr);
		return 0;
	}
	ConfNum = atoi(ConStr);
	TempConf = Find(ConfNum, "Dummy", 0);
	if (TempConf != NULL) {
		if (TempConf->PrevRecord != NULL)
			TempConf->PrevRecord->NextRecord = TempConf->NextRecord;
		if (TempConf->NextRecord != NULL)
			TempConf->NextRecord->PrevRecord = TempConf->PrevRecord;
		printf("Conference %d : %s deleted from your .cflist\n",
		       ConfNum, TempConf->ConfName);
		free(TempConf->ConfName);
		free(TempConf);
	} else {
		printf("Cannot Find Conference Number \"%d\"\n", ConfNum);
		return 1;
	}
	Shuffle();
	WriteTocflist();
	return 1;
}

int
DispMenu(void)
{
	int Choice;
	unsigned char Str[10];

	do {
		printf("\n1. Add");
		printf("\n2. Change");
		printf("\n3. Delete");
		printf("\n4. List");
		printf("\n5. Sort");
		printf("\n6. Quit");
		printf("\n\nEnter the Choice : ");
		GetString(Str, 5);
		Choice = atoi(Str);

	} while (Choice < 1 || Choice > 6);
	return Choice;
}

struct ConfList *
Find(int ConfNum, unsigned char *ConfName, int WhichOne)
{
	/* WhichOne = 0 ConfNum is compared  */
	/* WhichOne = 1 ConfName is compared */

	ConfList *TempConf;

	TempConf = Conf->NextRecord;
	while (TempConf != NULL) {
		if (!WhichOne) {
			if (TempConf->ConfNum == ConfNum)
				return TempConf;
		} else {
			if (!strcmp(TempConf->ConfName, ConfName))
				return TempConf;
		}
		TempConf = TempConf->NextRecord;
	}
	return NULL;
}

void
Shuffle(void)
{
	ConfList *TempConf;
	int Num = 0;

	TempConf = Conf->NextRecord;
	while (TempConf != NULL) {
		TempConf->ConfNum = Num;
		Num++;
		TempConf = TempConf->NextRecord;
	}
}

int
Add(void)
{
	PublicConf *Pub;
	char Str[10], ConStr[40], ConfName[40], ConfBuf[40], Yes;
	int ConfNum, Len, Ret, Index = 0, TempLen, Choice, CatNum;

	Pub = NULL;
	Choice = SelectList();
	if (Choice == 3)
		return 0;
	if (Choice == 1)
		DispAllPublicConfs();
	else if (Choice == 2) {
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
	}
	printf("Enter the conference numbers with space inbetween\n");
	printf("to be added to your .cflist file or 'q' to quit  :");
	GetString(ConStr, 35);
	if (ConStr[0] == 'q')
		return 0;
	if (Isblank(ConStr)) {
		printf("Conf num not entered quitting..\n");
		return 0;
	}
	Len = strlen(ConStr);
	while (Index < Len) {
		ExtractTokens(ConStr, &Index, ConfBuf);
		if (Isblank(ConfBuf))
			continue;
		ConfNum = atoi(ConfBuf);
		Ret = GetConfFromPublicTxt(Pub, ConfNum, ConfName);
		/* Ret=0 Invalid conf number */
		if (!Ret)
			continue;
		if (Find(ConfNum, ConfName, 1) != NULL) {
			printf("\n\"%s\" already exists in \".cflist\"\n", ConfName);
			printf("Do u still wanna add it press(y/n) : ");
			Yes = getchar();
			/* Store Newline */
			getc(stdin);
			if (Yes != 'y') {
				continue;
			}
		}
		TempLen = strlen(ConfName);
		WriteToBuffers(ConfName, ConfNum, TempLen);
		printf("\nConference %d : %s added to your .cflist\n",
		       ConfNum, ConfName);
		Shuffle();
		WriteTocflist();
	}
	return 1;
}

void
FreeMemory(void)
{
	FreeList();
	FreePublicConf();
}

int
Change(void)
{
	ConfList *TempConf;
	unsigned char Str[100], ConfName1[200], ConfName2[200], Yes;
	int ConfNum1, ConfNum2, Ret, Len;

	DisplayList();
	printf("Enter the conference number to change or 'q' to quit :");
	GetString(Str, 5);
	if (Str[0] == 'q')
		return 0;
	if (Isblank(Str)) {
		printf("\nConference num not entered Quitting...\n");
		return 0;
	}
	ConfNum1 = atoi(Str);
	TempConf = Find(ConfNum1, ConfName1, 0);
	if (TempConf == NULL) {
		printf("Invalid conference number\n");
		return 0;
	}
	strlcpy(ConfName1, TempConf->ConfName, sizeof(ConfName1));
	DispAllPublicConfs();
	printf("Changing \"%s\" in \".cflist\" file...\n", ConfName1);
	printf("Enter the conference number or 'q' to quit : ");
	GetString(Str, 5);
	if (Str[0] == 'q')
		return 0;
	if (Isblank(Str)) {
		printf("\nConference num not entered Quitting...\n");
		return 0;
	}
	ConfNum2 = atoi(Str);
	Ret = GetConfFromPublicTxt((PublicConf *) NULL, ConfNum2, ConfName2);
	/* Ret=0 Invalid conf number */
	if (!Ret)
		return 0;
	if (Find(ConfNum2, ConfName2, 1) != NULL) {
		printf("Conference \"%s\" already exists in \".cflist\"\n", ConfName2);
		printf("Still want to change \"%s\" by \"%s\" Press(y/n) : ", ConfName1, ConfName2);
		Yes = getchar();
		/* Store newline */
		getc(stdin);
		if (Yes != 'y')
			return 0;
	}
	Len = strlen(ConfName2);
	free(TempConf->ConfName);
	TempConf->ConfName = (char *) malloc(Len + 1);
	if (TempConf->ConfName == NULL) {
		printf("Cannot Allocate Memory\n");
		FreeMemory();
		exit(0);
	}
	strlcpy(TempConf->ConfName, ConfName2, sizeof(ConfName2));
	WriteTocflist();
	printf("\nConference %d : %s has been changed to %d : %s\n",
	       ConfNum1, ConfName1, ConfNum2, ConfName2);
	return 1;
}

void
WriteTocflist(void)
{
	FILE *Fp;
	ConfList *TempConf;
	char Buffer[200];
	int Len;

	if ((Fp = fopen(CflName, "w+")) == NULL) {
		printf("File \"%s\" doesn't exist:check directory\n", CflName);
		FreeMemory();
		exit(0);
	}
	TempConf = Conf->NextRecord;
	while (TempConf != NULL) {
		snprintf(Buffer, sizeof(Buffer), "%s\n", TempConf->ConfName);
		Len = strlen(Buffer);
		if (fwrite(Buffer, 1, Len, Fp) != Len) {
			printf("Cannot write to file \"%s\" check harddisk\n", CflName);
			FreeMemory();
			exit(0);
		}
		TempConf = TempConf->NextRecord;
	}
	fclose(Fp);
}

void
ExtractTokens(unsigned char *FileBuf, int *Index, unsigned char *ConfBuf)
{
	int j = 0;

	/* This is to filter spaces or commas in the start */
	while (FileBuf[*Index] == 0x20 || FileBuf[*Index] == ',' ||
	       FileBuf[*Index] == '\t')
		(*Index)++;
	do {
		if (FileBuf[*Index] == 0x20 || FileBuf[*Index] == ',' ||
		    FileBuf[*Index] == '\t' || FileBuf[*Index] == 0x0) {
			ConfBuf[j] = 0x0;
			break;
		}
		ConfBuf[j++] = FileBuf[(*Index)++];

	} while (0x9);
	/* Somebody may accidentally put a space or comma at the end */
	/* We have to overcome that */
	while (FileBuf[*Index] == 0x20 || FileBuf[*Index] == ',' ||
	       FileBuf[*Index] == '\t')
		(*Index)++;

}

/* The sorting method used is "bubble sort" which is easy to implement */
/* but not that effecient. */

void
Sort(void)
{
	int Choice;
	char Str[10];

	do {
		printf("\n1. Sort Alphabetically");
		printf("\n2. Swap any 2 conferences");
		printf("\n3. Move the conference");
		printf("\n4. Quit");
		printf("\n\nEnter the Choice : ");
		GetString(Str, 5);
		Choice = atoi(Str);

	} while (Choice < 1 || Choice > 4);
	switch (Choice) {
	case 1:
		SortAll();
		break;

	case 2:
		Swap();
		break;

	case 3:
		Move();
		break;

	case 4:
		break;
	}

}

void
SortAll(void)
{
	ConfList *Conf1, *Conf2, *a;
	int Swapped = 0;

	Conf1 = Conf->NextRecord;
	while (Conf1 != NULL) {
		Conf2 = Conf1->NextRecord;
		while (Conf2 != NULL) {
			if (strcmp(Conf1->ConfName, Conf2->ConfName) > 0) {
				SwapNodes(Conf1, Conf2);
				a = Conf1;
				Conf1 = Conf2;
				Conf2 = a;
				Swapped = 1;
			}
			Conf2 = Conf2->NextRecord;
		}
		Conf1 = Conf1->NextRecord;
	}
	/* Check if swapping is done and then write to .cflist */
	if (Swapped) {
		Shuffle();
		WriteTocflist();
		printf("\n\".cflist\" is sorted\n\n");
	} else {
		printf("\n\".cflist\" is already in sorting order\n\n");
	}
}

int
Swap(void)
{
	ConfList *Conf1, *Conf2;
	int Node1, Node2, Ret;
	char ConStr[30];

	DisplayList();
	printf("Enter the 2 conf numbers to be swapped\n");
	printf("with spaces inbetween or 'q' to quit : ");
	GetString(ConStr, 25);
	if (ConStr[0] == 'q')
		return 0;
	if (Isblank(ConStr)) {
		printf("Conf num not entered quitting..\n");
		return 0;
	}
	Ret = sscanf(ConStr, "%d %d", &Node1, &Node2);
	if (Ret != 2) {
		printf("Invalid Number of Arguments\n");
		return 0;
	}
	if (Node1 == Node2) {
		printf("\nNumbers cannot be the same\n");
		return 0;
	}
	Conf1 = Find(Node1, "Dummy", 0);
	if (Conf1 != NULL) {
		Conf2 = Find(Node2, "Dummy", 0);
		if (Conf2 != NULL) {
			SwapNodes(Conf1, Conf2);
			printf("Conference %d : %s and Conference %d : %s are Swapped\n",
			       Conf1->ConfNum, Conf1->ConfName,
			       Conf2->ConfNum, Conf2->ConfName);
			Shuffle();
			WriteTocflist();
		} else {
			printf("Conf with number \"%d\" not Found\n", Node2);
		}
	} else {
		printf("Conf with number \"%d\" not Found\n", Node1);
	}
	return 1;
}

void
SwapNodes(ConfList * Conf1, ConfList * Conf2)
{
	ConfList *a;

	if (Conf1->PrevRecord != NULL)
		Conf1->PrevRecord->NextRecord = Conf2;
	if (Conf2->NextRecord != NULL)
		Conf2->NextRecord->PrevRecord = Conf1;

	/* Check if the nodes are adjacent */

	if (Conf1->NextRecord == Conf2) {
		Conf2->PrevRecord = Conf1->PrevRecord;
		Conf1->PrevRecord = Conf2;
		Conf1->NextRecord = Conf2->NextRecord;
		Conf2->NextRecord = Conf1;
	} else {
		Conf1->NextRecord->PrevRecord = Conf2;
		Conf2->PrevRecord->NextRecord = Conf1;
		a = Conf1->PrevRecord;
		Conf1->PrevRecord = Conf2->PrevRecord;
		Conf2->PrevRecord = a;
		a = Conf1->NextRecord;
		Conf1->NextRecord = Conf2->NextRecord;
		Conf2->NextRecord = a;
	}

}

void
List(void)
{
	int Choice;
	char Str[10];

	do {
		printf("\n1. List conferences in your \".cflist\"");
		printf("\n2. Short listing of all conferences in \"Grex\"");
		printf("\n3. Long listing of all conferences in \"Grex\"");
		printf("\n4. Quit");
		printf("\n\nEnter the Choice : ");
		GetString(Str, 5);
		Choice = atoi(Str);

	} while (Choice < 1 || Choice > 4);
	switch (Choice) {
	case 1:
		DisplayList();
		break;

	case 2:
		DispAllPublicConfs();
		break;

	case 3:
		LongList();
		break;

	case 4:
		break;
	}

}

void
Ctrl_C_Handler(int sig)
{
	FreeMemory();
	printf("\n\nCtrl-C Encountered...\n");
	printf("Exiting the program...\n");
	printf("\nThanks for using .cflist builder program \"%s\". Bye!!!\n\n", User);
	exit(0);
}
