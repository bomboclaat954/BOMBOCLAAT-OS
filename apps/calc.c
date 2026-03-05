// BOMBOCLAAT-OS CALCULATOR
// Works but not perfect
// TODO: multiple expressions (like 2*2*2*2), numbers with decimal point
#include "../include/string.h"
#include "../include/screen.h"
#include "../include/api.h"
#include "../include/keyboard.h"
#include "../include/io.h"

int pow(int base, int exponent)
{
    double res = 1;
    for (int i = 0; i < exponent; i++)
    {
        res = res * base;
    }
    return res;
}

int find_operator(char *x, char *op)
{
    for (int i = 0; x[i]; i++)
    {
        if (x[i] == '+' || x[i] == '-' || x[i] == '*' || x[i] == '/' || x[i] == '^' || x[i] == '%')
        {
            *op = x[i];
            return i;
        }
    }
    return -1;
}

void calc_main(char *x)
{
    int already_printed = 0;
    char op;
    int op_pos = find_operator(x, &op);

    if (op_pos == -1)
    {
        puts("No operator found\n", 0);
        return;
    }

    char left[64];
    char right[64];

    for (int i = 0; i < op_pos; i++)
        left[i] = x[i];
    left[op_pos] = 0;

    int j = 0;
    for (int i = op_pos + 1; x[i]; i++)
        right[j++] = x[i];
    right[j] = 0;

    int a = atoi(left);
    int b = atoi(right);
    double result = 0;

    switch (op)
    {
    case '+':
        result = a + b;
        break;
    case '-':
        result = a - b;
        break;
    case '*':
        result = a * b;
        break;
    case '/':
        if (b == 0)
        {
            puts("Division by zero\n", 0);
            return;
        }
        result = a / b;
        break;
    case '^':
        result = pow(a, b);
        break;
    case '%':
        result = ((double)a / (double)100) * b;
        char *buf;
        dtoa(result, buf, 2);
        puts(buf, 1);
        already_printed = 1;
        break;
    }

    if (!already_printed)
    {
        char *res;
        itoa(result, res, 10);
        puts(res, 1);
    }
}
