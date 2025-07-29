#ifndef OPENSD_CAPI_H
#define OPENSD_CAPI_H

int opensd_init(int argc, char* argv[], const void* intracomm);
int opensd_run();
int opensd_simulation_finalize();
int opensd_simulation_init();
int opensd_reset();
int opensd_finalize();

// Global variables
extern char opensd_err_msg[256];

#endif // OPENSD_CAPI_H
