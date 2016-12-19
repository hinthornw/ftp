#define _GNU_SOURCE     /* To get defns of NI_MAXSERV and NI_MAXHOST */
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/if_link.h>

int main(int argc, char *argv[])
{
struct ifaddrs *ifaddr, *ifa;
int family, s, n;
char host[NI_MAXHOST];

if (getifaddrs(&ifaddr) == -1) {
perror("getifaddrs");
exit(EXIT_FAILURE);
}

/* Walk through linked list, maintaining head pointer so we
can free list later */

for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) {
    if (ifa->ifa_addr == NULL)
       continue;

    family = ifa->ifa_addr->sa_family;

    /* Display interface name and family (including symbolic
      form of the latter for the common families) */

    printf("%-8s %s (%d)\n",
          ifa->ifa_name,
          (family == AF_PACKET) ? "AF_PACKET" :
          (family == AF_INET) ? "AF_INET" :
          (family == AF_INET6) ? "AF_INET6" : "???",
          family);

    /* For an AF_INET* interface address, display the address */

    if (family == AF_INET || family == AF_INET6) {
       s = getnameinfo(ifa->ifa_addr,
               (family == AF_INET) ? sizeof(struct sockaddr_in) :
                                     sizeof(struct sockaddr_in6),
               host, NI_MAXHOST,
               NULL, 0, NI_NUMERICHOST);
       if (s != 0) {
           printf("getnameinfo() failed: %s\n", gai_strerror(s));
           exit(EXIT_FAILURE);
       }

       printf("\t\taddress: <%s>\n", host);

    } else if (family == AF_PACKET && ifa->ifa_data != NULL) {
       struct rtnl_link_stats *stats = ifa->ifa_data;

       printf("\t\ttx_packets = %10u; rx_packets = %10u\n"
              "\t\ttx_bytes   = %10u; rx_bytes   = %10u\n",
              stats->tx_packets, stats->rx_packets,
              stats->tx_bytes, stats->rx_bytes);
    }
}

freeifaddrs(ifaddr);
exit(EXIT_SUCCESS);
}

// #include <stdio.h>
// #include <string.h>
// #include <sys/types.h>
// #include <sys/socket.h>
// #include <netdb.h>
// #include <arpa/inet.h>
// #include <unistd.h>
// #include <netinet/in.h>

// int main(int argc, char *argv[])
// {
//     struct addrinfo hints, *res, *p;
//     int status;
//     char hostname[200];
//     char ipstr[INET6_ADDRSTRLEN];
//     struct ifaddrs * ifap;
//     //struct hostent
//     //char * PORT = "3490";
//     if (argc != 2) {
//         fprintf(stderr,"usage: showip hostname\n");
//         return 1;
//     }

//     gethostname(hostname, 200);
//     fprintf(stderr, "Hostname: (%s)\n", hostname);
//     //gethostbyname(hostname);

//     status = getifaddrs(&ifap);


//     memset(&hints, 0, sizeof hints);
//     hints.ai_family = AF_INET; // AF_INET or AF_INET6 to force version
//     hints.ai_socktype = SOCK_STREAM;
//     hints.ai_flags = AI_PASSIVE;

//     if ((status = getaddrinfo(hostname, NULL, &hints, &res)) != 0) {
//         fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
//         return 2;
//     }

//     printf("IP addresses for %s:\n\n", "me");

//     for(p = res;p != NULL; p = p->ai_next) {
//         void *addr;
//         char *ipver;

//         // get the pointer to the address itself,
//         // different fields in IPv4 and IPv6:
//         if (p->ai_family == AF_INET) { // IPv4
//             struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
//             addr = &(ipv4->sin_addr);
//             ipver = "IPv4";
//         } else { // IPv6
//             struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
//             addr = &(ipv6->sin6_addr);
//             ipver = "IPv6";
//         }

//         // convert the IP to a string and print it:
//         inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
//         printf("  %s: %s\n", ipver, ipstr);
//     }

//     freeaddrinfo(res); // free the linked list

//     return 0;
// }