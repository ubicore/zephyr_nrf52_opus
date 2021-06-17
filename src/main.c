/*
 * main.c
 *
 *  Created on: 12 mars 2019
 *      Author: nlantz
 */


#include <zephyr.h>
#include <stdio.h>
#include <version.h>
#include <shell/shell.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#include "radio.h"
#include "clock.h"

#ifdef CONFIG_AUDIO_CODEC_SGTL5000
#define AUDIO 1
#define RADIO 1
#endif

#if CONFIG_DISPLAY
#include "gui.h"
#endif


static struct k_sem quit_lock;

void quit(void)
{
	LOG_ERR("quit");
	k_sem_give(&quit_lock);
}


void panic(const char *msg)
{
	if (msg) {
		LOG_ERR("%s", msg);
	}

	for (;;) {
		k_sleep(K_FOREVER);
	}
}


/****************************************************************/
extern int storage_Init (void);

#include "user_led.h"
void main(void)
{
	LOG_INF("-- Zephyr:\t %s ",KERNEL_VERSION_STRING);
	LOG_INF("-- Board:\t %s", CONFIG_BOARD);
	LOG_INF("-- Compiled:\t %s %s", __DATE__, __TIME__);
	LOG_INF("Starting...");

	Led_Init();

	k_sem_init(&quit_lock, 0, UINT_MAX);

#if CONFIG_DISPLAY
	gui_preload();
#endif

#if RADIO
	//Init Radio Interface
	radio_Init();
#endif

#if AUDIO
	audio_server_init();
#endif

#if CONFIG_FILE_SYSTEM
	storage_Init();
#endif

	//Set Mode
	radio_mode_select(MODE_IDLE);
	//
    //shell_execute_cmd(NULL, "go");

#if CONFIG_DISPLAY
	gui_load();
#endif

	LOG_INF("wait App End...");
	k_sem_take(&quit_lock, K_FOREVER);
	LOG_INF("Stopping...");

#if AUDIO
	audio_server_stop();
#endif
}

#include <shell/shell.h>
#include <version.h>


static int cmd_version(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(shell, "Zephyr version %s", KERNEL_VERSION_STRING);

	return 0;
}


SHELL_CMD_ARG_REGISTER(version, NULL, "Show kernel version", cmd_version, 1, 0);


