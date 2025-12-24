#include <stdio.h>


void usage_logo() {
    printf(
        "\x1b[1;37m   _\x1b[0m\n"
        "\x1b[1;31m  ( \\                ..-----..__\x1b[0m\n"
        "\x1b[1;31m   \\.'.        _.--'`  [   '  ' ```'-._\x1b[0m\n"
        "\x1b[1;32m    `. `'-..-'' `    '  ' '   .  ;   ; `-'''-.,__/|/_\x1b[0m\n"
        "\x1b[1;32m      `'-.;..-''`|'  `.  '.    ;     '  `    '   `'  `,\x1b[0m\n"
        "\x1b[1;33m                 \\ '   .    ' .     '   ;   .`   . ' 7 \\\x1b[0m\n"
        "\x1b[1;33m                  '.' . '- . \\    .`   .`  .   .\\     `Y\x1b[0m\n"
        "\x1b[1;34m                    '-.' .   ].  '   ,    '    /'`\"\"';:'\x1b[0m\n"
        "\x1b[1;34m                      /Y   '.] '-._ /    ' _.-'\x1b[0m\n"
        "\x1b[1;35m                      \\'\\_   ; (`'.'.'  .\"/\"\x1b[0m\n"
        "\x1b[1;35m                       ' )` /  `.'   .-'.'\x1b[0m\n"
        "\x1b[1;36m                        '\\  \\).'  .-'--\"\x1b[0m\n"
        "\x1b[1;36m                          `. `,_'`\x1b[0m\n"
        "\x1b[1;37m                            `.__)\x1b[0m\n"
        "_   _ ____  ____      -----------------------------------------]\n"
        "| | | |  _ \\|  _ \\    https://github.com/Illusionna/UDPer      |\n"
        "| | | | | | | |_) |   can transmit quickly to upload your file.|\n"
        "| |_| | |_| |  __/    available version 1.0                    |\n"
        "\\___/|____/|_|        Illusionna (Zolio Marling) www@orzzz.net |\n"
        "---------------------------------------------------------------]\n"
    );
}


void usage_help(char *app_name, int port) {
    printf("1. Run        Run the server's udp service\n");
    printf("2. Config     Configure the connection server socket\n");
    printf("3. Send       Send file from the client to the server\n");
    printf("- (server) >>> \x1b[32m%s run [%d]\x1b[0m\n", app_name, port);
    printf("- (client) >>> \x1b[32m%s config [IPv4:%d]\x1b[0m\n", app_name, port);
    printf("- (client) >>> \x1b[32m%s send [file_path]\x1b[0m\n", app_name);
}


void usage_start(int pid, int port, char *local_ipv4) {
    printf("\x1b[32mINFO:\x1b[0m Started server process ID [\x1b[36m%d\x1b[0m]\n", pid);
    printf("\x1b[32mINFO:\x1b[0m Waiting for application startup.\n");
    printf("\x1b[32mINFO:\x1b[0m Application startup complete.\n");
    printf("\x1b[32mINFO:\x1b[0m C socket UDP service running on \x1b[36m%s:%d\x1b[0m (Press \x1b[33mCTRL+C\x1b[0m to quit)\n", local_ipv4, port);
}


void usage_end(int pid) {
    printf("\x1b[32mINFO:\x1b[0m Shutting down.\n");
    printf("\x1b[32mINFO:\x1b[0m Waiting for application shutdown.\n");
    printf("\x1b[32mINFO:\x1b[0m Application shutdown complete.\n");
    printf("\x1b[32mINFO:\x1b[0m Finished server process ID [\x1b[36m%d\x1b[0m]\n", pid);
}