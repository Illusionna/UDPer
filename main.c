#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "./utils/app.h"
#include "./utils/usage.inl"

#define DEFAULT_PORT 8080



int main(int argc, char *argv[], char *envs[]) {
    if (argc < 2) {
        usage_logo();
        usage_help(argv[0], DEFAULT_PORT);
        return 1;
    }

    char executable_dir[256];
    char executable_path[256];
    os_getexec(executable_dir, sizeof(executable_dir));
    snprintf(executable_path, sizeof(executable_path), "%s/%s", executable_dir, ".udp-config");

    if (strcmp(argv[1], "run") == 0) {
        if (argc == 2) {
            char local_ipv4[32];
            socket_ipv4(local_ipv4, sizeof(local_ipv4));
            usage_logo();
            usage_start(os_getpid(), DEFAULT_PORT, local_ipv4);
            app_server(DEFAULT_PORT);
            usage_end(os_getpid());
        }

        else if (argc == 3) {
            char *end;
            long port = strtol(argv[2], &end, 10);
            if (*end != '\0' || port <= 0) {
                printf("\x1b[1;31m[Error]:\x1b[0m \"%s\" is not a positive integer port.\n", argv[2]);
                return 1;
            } else {
                char local_ipv4[32];
                socket_ipv4(local_ipv4, sizeof(local_ipv4));
                usage_logo();
                usage_start(os_getpid(), port, local_ipv4);
                app_server(port);
                usage_end(os_getpid());
            }
        }

        else {
            printf("\x1b[1;31m[Error]:\x1b[0m at most one additional parameter.\n");
            printf(">>> \x1b[32m%s run [%d]\x1b[0m\n", argv[0], DEFAULT_PORT);
            return 1;
        }
    }

    else if (strcmp(argv[1], "send") == 0) {
        if (os_access(executable_path) != 1) {
            printf("\x1b[1;31m[Error]:\x1b[0m configure the remote server first.\n");
            printf(">>> %s config [ipv4:port]\n", argv[0]);
            return 1;
        }
        if (argc == 2 || argc > 3) {
            printf("\x1b[1;31m[Error]:\x1b[0m please input the file path.\n");
            printf(">>> \x1b[32m%s send [file_path]\x1b[0m\n", argv[0]);
            return 1;
        } else {
            char host_ipv4[16];
            int host_port;
            if (app_read_config(host_ipv4, &host_port) == 0) {
                return app_client(argv[0], argv[2], host_ipv4, host_port);
            } else {
                printf("\x1b[1;31m[Error]:\x1b[0m invalid \".udp-config\" file format in \"%s\".\n", executable_dir);
                printf("\x1b[32mCorrect format is like:\x1b[0m\n127.0.0.1\n8080\n");
                return 1;
            }
        }
    }

    else if (strcmp(argv[1], "config") == 0) {
        char host_ipv4[16];
        int host_port;

        if (argc == 2) {
            if (os_access(executable_path) == 1) {
                if (app_read_config(host_ipv4, &host_port) == 0) {
                    printf("IPv4: %s\nPort: %d\n", host_ipv4, host_port);
                } else {
                    printf("\x1b[1;31m[Error]:\x1b[0m invalid \".udp-config\" file format in \"%s\".\n", executable_dir);
                    printf("\x1b[32mCorrect format is like:\x1b[0m\n127.0.0.1\n8080\n");
                    return 1;
                }
            } else {
                printf("\x1b[1;31m[Error]:\x1b[0m no configuration file\n");
                printf(">>> %s config [ipv4:port]\n", argv[0]);
                return 1;
            }
        }

        else if (argc == 3) {
            int result = app_parse(argv[2], host_ipv4, &host_port);
            if (result == 1) printf("\x1b[1;33m[Warning]:\x1b[0m invalid socket (default configuration).\n");
            app_write_config(host_ipv4, host_port);
            printf("\x1b[1;32m[Success]:\x1b[0m \".udp-config\" is written to \"%s\".\n", executable_dir);
            return 0;
        }

        else {
            printf("\x1b[1;31m[Error]:\x1b[0m configuration display or parsing failed.\n");
            printf(">>> \x1b[32m%s config [socket]\x1b[0m\n", argv[0]);
            return 1;
        }
    }

    else {
        printf("\x1b[1;31m[Error]:\x1b[0m unknown flag parameter.\n");
        printf("1. Run        Run the server's udp service\n");
        printf("2. Config     Configure the connection server socket\n");
        printf("3. Send       Send file from the client to the server\n");
        return 1;
    }

    return 0;
}