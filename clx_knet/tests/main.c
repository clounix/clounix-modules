#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <termios.h>
#include <sys/types.h>
#include <getopt.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <hal_lightning_pkt_knl.h>
#include <clx_types.h>
#include <osal/osal_mdc.h>
#include <osal/osal.h>

int       interrupt   = 0;
UI32_T    verbosity   = 0;

void usage(char *prog)
{
    printf("%s usage:\n", prog);
    printf("          -i <interrupt>,  Interrupt test\n");
    printf("          -V <verbose>,    verbosity \n");
    printf("                           The default is %d \n", verbosity);
    exit(0);
}

void parse_cli(int argc, char **argv)
{
    int c;
    opterr = 0;
    int option_index = 0;

    struct option long_options[] =
    {
        {"verbosity",       no_argument,       0,          'v'},
        {"interrupt",       required_argument, 0,          'i'},
        {"help",            no_argument,       0,          'h'},
        {0, 0, 0, 0}
    };
    
    while(1)
    {
        option_index = 0;
        c = getopt_long (argc, argv, "i:vh",
                long_options, &option_index);
        if (c == -1)
            break;

        switch (c)
        {
            case 'i':
                interrupt = strtoul(optarg, NULL, 0);;
                break;
            case 'v':
                verbosity = 3;
                break;
            case '?':
            case 'h':
                usage(argv[0]);
            default:
                if (isprint(optopt))
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf(stderr,
                            "Unknown option character `\\x%x'.\n",
                            optopt);
                usage(argv[0]);
        }
    }
}


int main(int argc, char **argv) 
{
    CLX_ERROR_NO_T              rc = CLX_E_OTHERS;
    parse_cli(argc, argv);
    rc = hal_lightning_pkt_initPktDrv(0);
    if(rc){
        printf("Failed to open clx_netif");
    }

    hal_lightning_pkt_deinitPktDrv(0);

    exit(EXIT_SUCCESS);
}

