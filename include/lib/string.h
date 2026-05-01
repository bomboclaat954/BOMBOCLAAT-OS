#ifndef STRING_H
#define STRING_H
#include <stdint.h>

int strlen(const char *str);
int strcmp(const char *s1, const char *s2);
void strcpy(char *s, char *p);
int atoi(char *str);
int contains(char *str, char c);
char *dtoa(double num, char *str, int precision);
char *itoa(long num, char *str, int base);
void reverse(char *str, int length);
void to_lower_case(char *str);
void to_upper_case(char *str);
char *join(char *str1, char *str2, char *output_str, int n);
int is_number(char *x);
void *clear_str(char *str);
void *input(char *buf, int len);
void *input_passwd(char *buf, int len);
char *strchr(const char *s, int c);
int index(char *str, char x);
void delete_char(char *str, int index);

#endif
