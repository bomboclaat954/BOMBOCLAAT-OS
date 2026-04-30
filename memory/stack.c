#include <memory/stack.h>

void stack_init(stack_t *stack_t)
{
    stack_t->top = -1;
}

int isEmpty(stack_t *stack_t)
{
    return stack_t->top == -1;
}

int isFull(stack_t *stack_t)
{
    return stack_t->top >= MAX_SIZE - 1;
}

void push(stack_t *stack_t, int value)
{
    if (isFull(stack_t))
        return;
    stack_t->arr[++stack_t->top] = value;
}

int pop(stack_t *stack_t)
{
    if (isEmpty(stack_t))
        return -1;

    int popped = stack_t->arr[stack_t->top];
    stack_t->top--;
    return popped;
}

int top(stack_t *stack_t)
{
    if (isEmpty(stack_t))
        return -1;
    return stack_t->arr[stack_t->top];
}
