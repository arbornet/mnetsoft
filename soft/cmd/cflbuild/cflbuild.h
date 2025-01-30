#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include <pwd.h>

/* ConfList is a linked list for .cflist file */

typedef struct ConfList {
	char *ConfName;
	int ConfNum;
	struct ConfList *NextRecord;
	struct ConfList *PrevRecord;
} ConfList;

/* PublicConf is a linked list for public.txt file */

typedef struct PublicConf {
	char *ConfName;
	int ConfNum;
	int SerialNum;
	struct PublicConf *NextRecord;
	struct PublicConf *PrevRecord;
} PublicConf;

typedef struct Category {
	char *Category;
	int CatNum;
	struct PublicConf *ConfPub;
	struct Category *NextRecord;
	struct Category *PrevRecord;
} Category;

/* These subroutines are in "cflbuild.c" */

int main(void);
void InitList(void);
void WriteToBuffers(unsigned char *, int, int);
int DisplayList(void);
void FreeList(void);
int Delete(void);
int DispMenu(void);
void Shuffle(void);
struct ConfList *Find(int, unsigned char *, int);
int Add(void);
void FreeMemory(void);
int Change(void);
void WriteTocflist(void);
void ExtractTokens(unsigned char *, int *, unsigned char *);
int Swap(void);
void SwapNodes(ConfList *, ConfList *);
void Sort(void);
void SortAll(void);
void List(void);
void Ctrl_C_Handler(int);

/* These subroutines are in "cflsub.c" */

void ReadPublicTxt(void);
void InitPublicConf(void);
struct Category *AllocCats(int);
struct PublicConf *AllocConfs(int);
int WriteToPublicConf(char *, char *, int, int, int, int);
struct Category *FindCat(int);
void DispCategories(void);
void DispAllPublicConfs(void);
PublicConf *DispPublicConf(int);
int SelectList(void);
void FreePublicConf(void);
int ExtractConfName(char *, char *);
int Isblank(char *);
int GetConfFromPublicTxt(PublicConf *, int, char *);
int Move(void);
void Insert(ConfList *, ConfList *);
int LongList(void);
void GetString(char *, int);
void PrintString(char *, int);

extern Category *CatList;
extern ConfList *Conf;
extern char CflName[200], User[128], Home[1024];
extern int PrintCount;
extern struct passwd *PassWord;
