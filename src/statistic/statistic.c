/*
 * statistic.c
 *
 *  Created on: 22 août 2019
 *      Author: nlantz
 */


#include <zephyr.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(stat, LOG_LEVEL_DBG);
#include <shell/shell.h>
#include <stdio.h>
#include <stdbool.h>

#include "radio.h"
#include "audio_srv.h"
#include "latency.h"

#define SHELL_MSG_UNKNOWN_PARAMETER	" unknown parameter: "

#define STATS_DEFAULT_PERIOD_S 10

typedef struct {
	int32_t period;
	bool enable;
}Stat_Struct_t;


static Stat_Struct_t data = {
		.period = STATS_DEFAULT_PERIOD_S,
		.enable = false,
};

static void stats_print()
{
	audio_stats_print();
	latency_stats_print();
}



static void my_work_handler(struct k_work *work)
{
	stats_print();
}

K_WORK_DEFINE(my_work, my_work_handler);

void my_timer_handler(struct k_timer *dummy)
{
    k_work_submit(&my_work);
}

K_TIMER_DEFINE(my_timer, my_timer_handler, NULL);


static int cmd_stat_period(const struct shell *shell, size_t argc,
                           char **argv)
{
	uint32_t period;
    int ret;

    if(argc == 2){
    	ret = sscanf(argv[1], "%u", &period);
    	if(ret != 1){
    		shell_error(shell, "%s:%s%s", argv[0],
    				SHELL_MSG_UNKNOWN_PARAMETER, argv[1]);
    		return -EINVAL;
    	}

    	if(period != data.period ){
        	data.period = period;

        	if(data.enable){
                shell_execute_cmd(shell, "halt");
                shell_execute_cmd(shell, "go");
        	}
    	}
    }


	shell_print(shell, "Stat period is : %d", data.period);
	return 0;
}

static int cmd_stat_go(const struct shell *shell, size_t argc,
                           char **argv)
{
	/* start periodic timer that expires once every second */
	data.enable = true;
	k_timer_start(&my_timer, K_SECONDS(data.period), K_SECONDS(data.period));
	shell_print(shell, "Start stat every %d s", data.period);
	return 0;
}


static int cmd_stat_halt(const struct shell *shell, size_t argc,
                           char **argv)
{
	/* stop timer */
	k_timer_stop(&my_timer);
	data.enable = false;
	shell_print(shell, "Stop stat");
	return 0;
}

static int cmd_stat_print(const struct shell *shell, size_t argc,
                           char **argv)
{
    k_work_submit(&my_work);
	return 0;
}


SHELL_STATIC_SUBCMD_SET_CREATE(m_sub_stat,
	SHELL_CMD_ARG(period, NULL, "get or set peiod for periodic stat" , cmd_stat_period, 1, 1),
	SHELL_CMD_ARG(go, NULL, "start periodic stat", cmd_stat_go, 1, 0),
	SHELL_CMD_ARG(halt, NULL, "stop periodic stat" , cmd_stat_halt, 1, 0),
	SHELL_SUBCMD_SET_END
);



/* Creating root (level 0) command "demo" without a handler */
SHELL_CMD_REGISTER(stat, &m_sub_stat, "Stat commands.", cmd_stat_print);

