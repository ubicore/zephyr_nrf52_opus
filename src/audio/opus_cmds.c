/*
 * opus_cmds.c
 *
 *  Created on: 8 août 2019
 *      Author: nlantz
 */


#include <shell/shell.h>

#include <stdio.h>
#include "codec_opus.h"
#include "opus.h"

static int cmd_opus_encoder_ctl(const struct shell *shell, size_t argc,
                           char **argv)
{
        int32_t cmd;
        int32_t param;
        int ret;

        ret = sscanf(argv[1], "%u", &cmd);
        if(ret != 1){
            shell_print(shell, "bad arg %s", argv[0]);
        	return 0;
        }

        if(cmd & 0x01)
        {
        	if(argc != 2){
        		shell_error(shell, "%s: Get request %d do not take parameter", argv[0], cmd);
        		return -EINVAL;
        	}

            if(opus_encoder_ctrl_fixed_arg(cmd, (uint32_t) &param) == OPUS_OK){
                     shell_print(shell, "OPUS_OK");

                     shell_print(shell, "GET %d is %d", cmd, param);

                 }else {
                     shell_print(shell, "OPUS_ERROR");
                 }


        } else{
        	if(argc != 3){
        		shell_error(shell, "%s: Set request %d must have one parameter", argv[0], cmd);
        		return -EINVAL;
        	}

            ret = sscanf(argv[2], "%u", &param);
            if(ret != 1){
                shell_print(shell, "bad arg %s", argv[1]);
            	return 0;
            }

            shell_print(shell, "SET %d to %d", cmd, param);


            if(opus_encoder_ctrl_fixed_arg(cmd, param) == OPUS_OK){
                shell_print(shell, "OPUS_OK");
            }else {
                shell_print(shell, "OPUS_ERROR");
            }
        }

        return 0;
}


/* Creating subcommands (level 1 command) array for command "demo". */
SHELL_STATIC_SUBCMD_SET_CREATE(sub_opus,
		SHELL_CMD_ARG(encoder_ctl, NULL, "<40xx> <value> see opus_define.h to get command number ", cmd_opus_encoder_ctl, 2, 1),
        SHELL_SUBCMD_SET_END /* Array terminated. */
);

/* Creating root (level 0) command "demo" without a handler */
SHELL_CMD_REGISTER(opus, &sub_opus, "Opus commands", NULL);



