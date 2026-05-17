/*
    BOMBOCLAAT-OS CALCULATOR
    TODO: multiple expressions (like 2*2*2*2), numbers with decimal point
*/
#include <lib/string.h>
#include <drivers/screen.h>
#include <bomboclaat-os/api.h>
#include <drivers/keyboard.h>
#include <drivers/io.h>
#include <lib/math.h>
#include <apps/calc.h>
#include <memory/kmalloc.h>

int find_operator(char *x, char *op)
{
    for (int i = 0; x[i]; i++)
    {
        if (x[i] == '+' || x[i] == '-' || x[i] == '*' || x[i] == '/' || x[i] == '^' || x[i] == '%' || x[i] == '$')
        {
            *op = x[i];
            return i;
        }
    }
    return -1;
}

void calc_main(char *x)
{
    if (contains(x, '$'))
    {
        char a[16];
        for (int i = 0; i < strlen(x); i++)
            a[i] = x[i + 1];

        int b = atoi(a);
        char res[16];
        float c = sqrt(b);
        dtoa(c, res, 4);
        puts(res, 1);
        return;
    }
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
            puts("Can't divide by zero", 1);
            return;
        }
        result = a / b;
        break;
    case '^':
        result = pow(a, b);
        break;
    case '%':
        result = ((double)a / (double)100) * b;
        char *buf = kmalloc(64);
        dtoa(result, buf, 2);
        puts(buf, 1);
        kfree(buf);
        already_printed = 1;
        break;
    }

    if (!already_printed)
    {
        char *res = kmalloc(64);
        itoa(result, res, 10);
        puts(res, 1);
        kfree(res);
    }
}
