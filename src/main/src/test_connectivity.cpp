#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "logging.h"
#include "connectivity.h"

int main (int argc, char* argv[])
{
    freopen ("/opt/logs/netsrvmgr.log", "a", stderr);
    freopen ("/opt/logs/netsrvmgr.log", "a", stdout);

    long timeout_ms, max_tries;
    if (argc < 2 || (timeout_ms = strtol(argv[1], NULL, 10)) <= 0)
        timeout_ms = 2000; // default to 2s if unspecified / non-positive
    if (argc < 3 || (max_tries = strtol(argv[2], NULL, 10)) <= 0)
        max_tries = 1; // default to 1 if unspecified / non-positive

    rdk_logger_init(0 == access("/opt/debug.ini", R_OK) ? "/opt/debug.ini" : "/etc/debug.ini");

    int connectivity;
    for (int i = 1; (connectivity = test_connectivity(timeout_ms)) <= 0 && i < max_tries; i++)
        sleep(1); // sleep 1s after a connectivity test failure before trying again

    fclose (stdout);
    fclose (stderr);

    return (connectivity > 0) ? 0 : 1; // connectivity test successful => exit status 0
}
