#include <stdio.h>
#include <malloc.h>

int main(int argc, char **argv)
{
    int fd = fopen("/etc/changelog", 0);
    if (fd < 0)
    {
        printf("cannot open changelog\n");
        return 1;
    }

    char *buf = malloc(sizeof(char) * 1024);

    int bytesRead = fread(fd, buf, 75);
    if (bytesRead < 0)
    {
        printf("read error\n");
        free(buf);
        fclose(fd);
        return 1;
    }

    buf[bytesRead] = '\0';

    printf("%s\n", buf);

    free(buf);
    fclose(fd);
    return 0;
}