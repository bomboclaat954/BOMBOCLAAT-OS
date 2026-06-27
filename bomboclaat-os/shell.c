/*
 * BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <bomboclaat.h>
#include <stdint.h>

int main()
{
    while (1)
    {
        char cmd_line[128];
        char cmd[32];
        char arg[96];

        printf("root@bomboclaat:~# ");
        scanf(cmd_line);

        int i = 0, j = 0;
        while (cmd_line[i] != ' ' && cmd_line[i] != '\0' && i < 31)
        {
            cmd[i] = cmd_line[i];
            i++;
        }
        cmd[i] = '\0';

        while (cmd_line[i] == ' ')
            i++;

        while (cmd_line[i] != '\0' && j < 95)
        {
            arg[j] = cmd_line[i];
            i++;
            j++;
        }
        arg[j] = '\0';

        char path[128];
        sprintf(path, "bin/%s", cmd);
        // parse and pass the arguments somehow

        if (strcmp(cmd_line, "\0") == 0)
            continue;
        else if (sysexec(path) == 0)
            printf("Unknown command\n");
    }
    return 0;
}
