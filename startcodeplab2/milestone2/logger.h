/**
 * \author BL
 */

#ifndef _LOGGER_H_
#define _LOGGER_H_

#include <stdio.h>
#include <stdlib.h>

int write_to_log_process(char *msg);
int create_log_process();
int end_log_process();

#endif