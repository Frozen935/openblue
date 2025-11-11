#ifndef ZEPHYR_SHIM_SHELL_H
#define ZEPHYR_SHIM_SHELL_H


/* Shell severity levels used in bt_shell_private */
#ifndef SHELL_INFO
#define SHELL_INFO 0
#endif
#ifndef SHELL_NORMAL
#define SHELL_NORMAL 0
#endif
#ifndef SHELL_WARNING
#define SHELL_WARNING 0
#endif
#ifndef SHELL_ERROR
#define SHELL_ERROR 0
#endif

#ifndef SHELL_CMD_ARG
#define SHELL_CMD_ARG(...) /* no-op */
#endif
#ifndef SHELL_CMD_HELP_PRINTED
#define SHELL_CMD_HELP_PRINTED 0
#endif
#ifndef SHELL_CMD_ARG_REGISTER
#define SHELL_CMD_ARG_REGISTER(...) /* no-op */
#endif
#ifndef SHELL_CMD
#define SHELL_CMD(name, subcmd, help, handler) /* no-op */
#endif
#ifndef SHELL_CMD_REGISTER
#define SHELL_CMD_REGISTER(name, subcmd, help, handler) /* no-op */
#endif
#ifndef SHELL_STATIC_SUBCMD_SET_CREATE
#define SHELL_STATIC_SUBCMD_SET_CREATE(name, ...) /* no-op */
#endif
#ifndef SHELL_SUBCMD_SET_END
#define SHELL_SUBCMD_SET_END /* no-op */
#endif
#ifndef __printf_like
#define __printf_like(a, b)
#endif
/* Placeholder shell type to satisfy prototypes */
struct shell { int _dummy; };
/* Shell color enum placeholder */
enum shell_vt100_color { SHELL_VT100_DEFAULT = 0 };


#endif /* ZEPHYR_SHIM_SHELL_H */