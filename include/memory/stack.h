#ifndef STACK_H
#define STACK_H

#define MAX_SIZE 1024

typedef struct
{
    int arr[MAX_SIZE];
    int top;
} stack_t;

void stack_init(stack_t *stack_t);
int isEmpty(stack_t *stack_t);
int isFull(stack_t *stack_t);
void push(stack_t *stack_t, int value);
int pop(stack_t *stack_t);
int top(stack_t *stack_t);

#endif