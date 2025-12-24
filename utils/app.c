#include "app.h"


static volatile sig_atomic_t RUNNING = 1;
static Socket S = SOCKET_INVALID;
static FILE *F = NULL;
static FILE *LOG = NULL;


void app_interrupt(int semaphore) {
    RUNNING = 0;
    if (S != SOCKET_INVALID) socket_close(S);
}


void app_convert(unsigned int value, char *buffer, int size) {
    if (value < 1024) {
        snprintf(buffer, size, "%u B", value);
    } else if (value < 1048576) {
        snprintf(buffer, size, "%.2f KB", value / 1024.0);
    } else if (value < 1073741824) {
        snprintf(buffer, size, "%.2f MB", value / 1024.0 / 1024.0);
    } else {
        snprintf(buffer, size, "%.2f GB", value / 1024.0 / 1024.0 / 1024.0);
    }
}


void app_progress(int current, int epoch, int step, char *description) {
    int message = (description != NULL && description[0] != '\0');
    if (!message && current != 0 && current % step != 0 && current != epoch) return;
    int width = 50;
    // printf("\x1b[?25l");
    float percentage = (epoch > 0) ? (float)current / epoch : 0.0F;
    int filled = percentage * width;
    printf("\r[");
    for (int j = 0; j < width; j++) {
        if (j < filled) printf("\x1b[32m=\x1b[0m");
        else if (j == filled) printf(">");
        else printf(" ");
    }
    printf("] %7.2f%% (%d/%d)", percentage * 100, current, epoch);
    if (message) printf(" \x1b[33m%s\x1b[0m\x1b[K", description);
    else printf("\x1b[K");
    fflush(stdout);
    if (current >= epoch) printf("\n");
    // if (current >= epoch) printf("\x1b[?25h\n");
}


int app_parse(char *address, char *ipv4, int *port) {
    strcpy(ipv4, "127.0.0.1");
    *port = 8080;
    if (!address) return 1;
    char ip[32];
    int number;
    if (sscanf(address, "%15[0-9.]:%d", ip, &number) == 2) {
        strcpy(ipv4, ip);
        *port = number;
        return 0;
    }
    return 1;
}


void app_write_config(char *ip, int port) {
    char executable_dir[256];
    char executable_path[256];
    os_getexec(executable_dir, sizeof(executable_dir));
    snprintf(executable_path, sizeof(executable_path), "%s/%s", executable_dir, ".udp-config");
    FILE *fp = fopen(executable_path, "w");
    if (fp == NULL) {
        perror("\x1b[1;31m[Error]\x1b[0m");
        return;
    }
    fprintf(fp, "%s\n%d", ip, port);
    fclose(fp);
}


int app_read_config(char *ip, int *port) {
    char executable_dir[256];
    char executable_path[256];
    os_getexec(executable_dir, sizeof(executable_dir));
    snprintf(executable_path, sizeof(executable_path), "%s/%s", executable_dir, ".udp-config");
    FILE *fp = fopen(executable_path, "r");
    if (fp == NULL) return 1;
    char ipv4[32];
    int number;
    if (fscanf(fp, "%15[0-9.] %d", ipv4, &number) != 2) {
        fclose(fp);
        return 1;
    }
    strcpy(ip, ipv4);
    *port = number;
    fclose(fp);
    return 0;
}


int app_server(int port) {
    signal(SIGINT, app_interrupt);

    LOG = fopen("log.log", "w");
    log_setting(1);
    log_config_write(LOG);

    if (socket_init()) {
        log_error("Initialize socket.");
        return 1;
    }

    S = socket_create(AF_INET, SOCK_DGRAM, 0);
    if (S == SOCKET_INVALID) {
        log_error("Create socket.");
        socket_destroy();
        return 1;
    }

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    socket_config(&server, AF_INET, "0.0.0.0", port);

    if (socket_bind(S, &server, sizeof(server)) == SOCKET_INVALID) {
        log_error("Bind socket.");
        socket_close(S);
        socket_destroy();
        return 1;
    }

    unsigned int received_packets = 0;
    unsigned int expected_sequence = 0;
    unsigned int cumulative_bytes = 0;
    unsigned int duplicate_packets = 0;
    unsigned int out_of_order_packets = 0;

    double start_time;
    double end_time;
    double elapsed_time;
    double speed_mbps;
    char information[BUFFER_SIZE];
    char filename[192];
    char fullpath[256];
    char filesize[32];
    char hash[33];

    while (RUNNING) {
        PacketUDP udpong;
        PacketACK udping;

        struct sockaddr_in client;
        int client_length = sizeof(client);

        int n = socket_recvfrom(S, &udpong, sizeof(udpong), 0, &client, &client_length);
        if (n < 0) {
            if (!RUNNING) break;
            log_warning("Receive socket.");
            continue;
        }

        udpong.sequence = socket_ntohl(udpong.sequence);
        udpong.length = socket_ntohl(udpong.length);

        // The maximum of `unsigned int` is `0xFFFFFFFF` denoting handshake connection protocol.
        if (udpong.sequence == 0xFFFFFFFF) {
            udping.acknowledge = socket_htonl(expected_sequence);
            udping.window_size = socket_htonl(WINDOW_SIZE);
            socket_sendto(S, &udping, sizeof(udping), 0, &client, client_length);
            continue;
        }

        // The response of UDP packet is done.
        if (udpong.length == 0 && udpong.sequence == expected_sequence) {
            log_trace("Client -> %s:%d", inet_ntoa(client.sin_addr), socket_ntohs(client.sin_port));

            // Conservative writing style.
            if (F != NULL) {
                fclose(F);
                F = NULL;
            }

            FILE *f = fopen(fullpath, "rb");
            md5_file(f, hash);
            fclose(f);

            end_time = os_time();
            elapsed_time = end_time - start_time;
            speed_mbps = (cumulative_bytes * 8.0) / elapsed_time / 1000000.0;

            app_convert(cumulative_bytes, filesize, sizeof(filesize));
            snprintf(
                information, sizeof(information),
                "Server information:\n - md5 hash: %s\n - average speed: %.2f Mbps\n - total UDP packets received: %u\n - duplicate packets: %u\n - out of order packets: %u\n - total size received: %s\n - transmission time: %.5f s",
                hash, speed_mbps, received_packets, duplicate_packets, out_of_order_packets, filesize, elapsed_time
            );
            log_info(information);

            for (int i = 0; i < MAX_RETRY; i++) {
                socket_sendto(S, &information, strlen(information) + 1, 0, &client, client_length);
                os_sleep(0.1);
            }

            received_packets = 0;
            expected_sequence = 0;
            cumulative_bytes = 0;
            duplicate_packets = 0;
            out_of_order_packets = 0;
            continue;
        }

        received_packets++;

        if (udpong.sequence == expected_sequence) {
            // The first UDP packet.
            if (udpong.sequence == 0) {
                start_time = os_time();
                memcpy(filename, udpong.data, udpong.length);
                filename[udpong.length] = '\0';
                // Conservative writing style.
                if (F != NULL) {
                    fclose(F);
                    F = NULL;
                }
                os_mkdir("cache");
                snprintf(fullpath, sizeof(fullpath), "cache/%s", filename);
                F = fopen(fullpath, "wb");
            } else {
                if (F != NULL) {
                    fwrite(udpong.data, 1, udpong.length, F);
                    cumulative_bytes = cumulative_bytes + udpong.length;
                }
            }
            expected_sequence++;
        }

        else if (udpong.sequence < expected_sequence) {
            duplicate_packets++;
            app_convert(cumulative_bytes, filesize, sizeof(filesize));
            // log_warning(
            //     "[SEQUENCE %u] < [EXPECTATION %u] | Received: %s | Duplicates: %u",
            //     udpong.sequence, expected_sequence, filesize, duplicate_packets
            // );
        }

        else {
            out_of_order_packets++;
            app_convert(cumulative_bytes, filesize, sizeof(filesize));
            // log_warning(
            //     "[SEQUENCE %u] > [EXPECTATION %u] | Received: %s | Out of order: %u",
            //     udpong.sequence, expected_sequence, filesize, out_of_order_packets
            // );
        }

        udping.acknowledge = socket_htonl(expected_sequence);
        udping.window_size = socket_htonl(WINDOW_SIZE);
        socket_sendto(S, &udping, sizeof(udping), 0, &client, client_length);
    }

    printf("\n\x1b[1;32m* Interrupt: Disconnect Socket and Free Memory.\x1b[0m\n");
    if (F != NULL) fclose(F);
    socket_destroy();
    fclose(LOG);
    return 0;
}


int app_handshake(Socket c, struct sockaddr_in *server, int server_size) {
    PacketUDP ping;
    PacketACK pong;

    ping.sequence = socket_htonl(0xFFFFFFFF);
    ping.length = socket_htonl(1314520);
    memset(ping.data, 0, BUFFER_SIZE);
    strcpy(ping.data, "PING");

    for (int i = 0; i < MAX_RETRY; i++) {
        socket_sendto(c, &ping, sizeof(ping), 0, server, server_size);
        int n = socket_recvfrom(c, &pong, sizeof(pong), 0, NULL, NULL);
        if (n > 0) return 0;
        else log_warning("Retry to send [%d / %d].", i + 1, MAX_RETRY);
    }
    return 1;
}


int app_client(char *app_name, char *filepath, char *ipv4, int port) {
    if (os_access(filepath) != 1) {
        log_fatal("File \"%s\" does not exist.", filepath);
        return 1;
    }

    double start_time = os_time();

    Time duration = {
        .second = 0,
        .microsecond = TIMEOUT_MS * 1000
    };

    char *basename = os_basename(filepath);
    if (os_isdir(filepath) || !basename) {
        log_error("\"%s\" is a directory.", filepath);
        return 1;
    }

    if (socket_init()) {
        log_error("Initialize socket.");
        return 1;
    }

    Socket c = socket_create(AF_INET, SOCK_DGRAM, 0);
    if (c == SOCKET_INVALID) {
        log_error("Create socket.");
        socket_destroy();
        return 1;
    }

    if (socket_setopt(c, SOL_SOCKET, SO_RCVTIMEO, &duration, sizeof(duration)) == SOCKET_INVALID) {
        log_error("Set socket port timeout failed.");
        socket_close(c);
        socket_destroy();
        return 1;
    }

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    socket_config(&server, AF_INET, ipv4, port);
    int server_size = sizeof(server);

    if (app_handshake(c, &server, server_size)) {
        log_error(
            "Is the server running on \"\x1b[33m%s:%d\x1b[0m\"?\nYou might need to use \"\x1b[1;32m%s run\x1b[0m\" or configure the remote server \"\x1b[1;32m%s config [ipv4:port]\x1b[0m\".", ipv4, port, app_name, app_name
        );
        socket_close(c);
        socket_destroy();
        return 1;
    }

    char description[128] = {0};
    unsigned int cumulative_packets = 1;
    unsigned int window_offset = 0;
    unsigned int next_sequence = 0;
    unsigned int window_size = WINDOW_SIZE;
    unsigned int cumulative_retransmission = 0;
    int retry = 0;

    unsigned int capacity = 100000;
    PacketUDP *cache = (PacketUDP *)malloc(sizeof(PacketUDP) * capacity);

    cache[0].sequence = 0;
    strncpy(cache[0].data, basename, BUFFER_SIZE - 1);
    cache[0].data[BUFFER_SIZE - 1] = '\0';
    cache[0].length = strlen(cache[0].data);

    char hash[33];
    FILE *fp = fopen(filepath, "rb");
    md5_file(fp, hash);
    fclose(fp);

    FILE *f = fopen(filepath, "rb");
    while (1) {
        if (cumulative_packets >= capacity) {
            capacity = capacity * 2;
            cache = (PacketUDP *)realloc(cache, sizeof(PacketUDP) * capacity);
        }
        unsigned int length = fread(cache[cumulative_packets].data, 1, BUFFER_SIZE, f);
        if (length == 0) break;
        cache[cumulative_packets].sequence = cumulative_packets;
        cache[cumulative_packets].length = length;
        cumulative_packets++;
    }
    fclose(f);

    double last_time = os_time();

    while (window_offset < cumulative_packets) {
        while (next_sequence < window_offset + window_size && next_sequence < cumulative_packets) {
            PacketUDP udping = cache[next_sequence];
            udping.sequence = socket_htonl(udping.sequence);
            udping.length = socket_htonl(udping.length);
            socket_sendto(c, &udping, sizeof(udping), 0, &server, server_size);
            next_sequence++;
        }

        PacketACK udpong;
        int n = socket_recvfrom(c, &udpong, sizeof(udpong), 0, NULL, NULL);

        if (n > 0) {
            unsigned int acknowledge = socket_ntohl(udpong.acknowledge);
            if (acknowledge > window_offset) {
                window_offset = acknowledge;
                window_size = socket_ntohl(udpong.window_size);
                retry = 0;
                last_time = os_time();
                app_progress(window_offset, cumulative_packets, window_size * 10, NULL);
            }
        } else {
            if ((os_time() - last_time) * 1000 > TIMEOUT_MS) {
                retry++;
                if (retry > MAX_RETRY) {
                    printf("\n\x1b[1;31m[Error]:\x1b[0m UDP data packet can not reach server.\n");
                    free(cache);
                    socket_close(c);
                    socket_destroy();
                    return 1;
                }

                snprintf(description, sizeof(description), "Timeout causes to retransmit [SEQ %u] and retry to send [%d / %d].", window_offset, retry, MAX_RETRY);
                app_progress(window_offset, cumulative_packets, window_size * 10, description);

                next_sequence = window_offset;
                cumulative_retransmission++;
                last_time = os_time();
            }
        }
    }

    PacketUDP request;
    memset(&request, 0, sizeof(request));
    request.sequence = socket_htonl(cumulative_packets);
    request.length = 0;

    retry = 0;
    int ok = 1;

    while (retry < MAX_RETRY) {
        socket_sendto(c, &request, sizeof(request), 0, &server, server_size);
        char response[BUFFER_SIZE];
        int n = socket_recvfrom(c, &response, sizeof(response), 0, NULL, NULL);
        if (n > 0) {
            if (n <= sizeof(PacketACK)) continue;

            if (n < BUFFER_SIZE) response[n] = '\0';
            else response[BUFFER_SIZE - 1] = '\0';

            if (strlen(response) > 0) {
                ok = 0;
                double end_time = os_time();
                printf("%s\n", response);
                printf("Client information:\n");
                printf(" - md5 hash: %s\n", hash);
                printf(" - total UDP packets sent: %u\n", cumulative_packets);
                printf(" - retransmission packets: %u\n", cumulative_retransmission);
                printf(" - running time: %.5f s\n", end_time - start_time);
                break;
            }
        }
        retry++;
        os_sleep(0.1);
    }
    if (ok) log_warning("No acknowledge from server (timeout).");

    free(cache);
    socket_close(c);
    socket_destroy();
    return 0;
}