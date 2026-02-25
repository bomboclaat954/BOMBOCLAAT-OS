#ifndef STRING_H
#define STRING_H

int strlen(const char *str);
int strcmp(const char *s1, const char *s2);
int atoi(char *str);
char *itoa(unsigned long num, char *str, int base);
void reverse(char *str, int length);
void to_lower_case(char *str);
void to_upper_case(char *str);
char *join(char *str1, char *str2, char *output_str, int n);
int is_number(char *x);

#endif
