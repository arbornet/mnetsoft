/* (c) 2003, Jan D. Wolter, All Rights Reserved. */

char *get_noise();

#ifdef MAKE_NOISE
void make_noise(char *stuff1, int len1, char *stuff2, len2);
#else
#define make_noise(s1,l1,s2,l2)
#endif
