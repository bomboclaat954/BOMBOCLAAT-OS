// BOMBOCLAAT-OS STRING LIBRARY
// Some string tools
#include "../include/string.h"

int strlen(const char *str)
{
    int len = 0;
    while (str[len] != '\0')
    {
        len++;
    }
    return len;
}

int strcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

void to_lower_case(char *str)
{
    for (char *p = str; *p; ++p)
        *p = *p > 0x40 && *p < 0x5B ? *p | 0x60 : *p;
}

void to_upper_case(char *str)
{
    for (char *p = str; *p; ++p)
        *p = *p > 0x60 && *p < 0x7B ? *p & ~0x20 : *p;
}

int atoi(char *str) // ascii to int
{
    int res = 0;
    while (*str >= '0' && *str <= '9')
    {
        res = res * 10 + (*str - '0');
        str++;
    }
    return res;
}

int contains(char *str, char c)
{
    for (int i = 0; str[i]; i++)
    {
        if (str[i] == c)
        {
            return 1;
            break;
        }
        else
        {
            continue;
        }
    }
    return 0;
}

char *dtoa(double num, char *str, int precision)
{
    int i = 0;

    if (num < 0)
    {
        str[i++] = '-';
        num = -num;
    }

    long int_part = (long)num;

    char int_str[32];
    itoa(int_part, int_str, 10);

    int j = 0;
    while (int_str[j])
        str[i++] = int_str[j++];

    str[i++] = '.';

    double frac = num - (double)int_part;

    for (int k = 0; k < precision; k++)
    {
        frac *= 10.0;
        int digit = (int)frac;

        str[i++] = digit + '0';

        frac -= digit;
    }

    str[i] = '\0';

    return str;
}

char *itoa(long num, char *str, int base) // int to ascii
{
    int i = 0;
    int is_negative = 0;

    if (num == 0)
    {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }

    if (num < 0 && base == 10)
    {
        is_negative = 1;
        num = -num;
    }

    while (num != 0)
    {
        int rem = num % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        num = num / base;
    }

    if (is_negative)
    {
        str[i++] = '-';
    }

    str[i] = '\0';
    reverse(str, i);
    to_upper_case(str);
    return str;
}

void reverse(char *str, int length)
{
    int start = 0;
    int end = length - 1;
    while (start < end)
    {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

char *join(char *str1, char *str2, char *output_str, int n)
{
    int len1 = strlen(str1);
    int len2 = strlen(str2);
    for (int i = 0; i < len1; i++)
        output_str[i] = str1[i];
    for (int i = 0; i < len2; i++)
        output_str[len1 + i] = str2[i];
    int start = len1 + len2;
    if (n)
        output_str[start++] = '\n';
    output_str[start] = '\0';
    return output_str;
}

int is_number(char *x)
{
    int x_len = strlen(x);
    for (int i = 0; i < x_len; i++)
    {
        if (x[i] == '0' || x[i] == '1' || x[i] == '2' || x[i] == '3' || x[i] == '4' ||
            x[i] == '5' || x[i] == '6' || x[i] == '7' || x[i] == '8' || x[i] == '9')
            return 1;
        else
            return 0;
    }
}
