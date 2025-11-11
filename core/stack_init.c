/*
 * STACK_INIT registry traversal (section-based aggregator)
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "bt_stack_init.h"

/* Start/Stop symbols provided by GNU ld for the section named exactly
 * as used in the attribute. No custom linker script required.
 */
extern const struct stack_init_entry __start_stack_init[];
extern const struct stack_init_entry __stop_stack_init[];

extern const struct stack_init_entry __stack_init_entry_main_work_init;
extern const struct stack_init_entry __stack_init_entry_uc_init;
int bt_stack_init_once(void)
{
	__stack_init_entry_main_work_init.init();
	__stack_init_entry_uc_init.init();

	return 0;
}
