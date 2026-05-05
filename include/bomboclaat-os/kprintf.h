#ifndef KPRINTF_H
#define KPRINTF_H

#ifdef __cplusplus
extern "C"
{
#endif

    int kprintf(const char *fmt, ...);
    int sprintf(char *buf, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif
