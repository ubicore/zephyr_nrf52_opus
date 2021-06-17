/*
 * audio_cmds.c
 *
 *  Created on: 8 août 2019
 *      Author: nlantz
 */

#include <shell/shell.h>
#include <stdio.h>
#include <stdbool.h>
#include "config.h"
#include "drv_sgtl5000.h"


#define SHELL_MSG_UNKNOWN_PARAMETER	" unknown parameter: "

static int cmd_audio_mute_get(const struct shell *shell, size_t argc,
                           char **argv)
{
	shell_print(shell, "Mute is: %s",
			(audio_mute_get()) ? "On" : "Off");
    return 0;
}

static int cmd_audio_mute(const struct shell *shell, size_t argc,
                           char **argv)
{
	audio_mute_set(true);
   	shell_print(shell, "mute");
    return 0;
}

static int cmd_audio_unmute(const struct shell *shell, size_t argc,
                           char **argv)
{
	audio_mute_set(false);
    shell_print(shell, "un-mute");
    return 0;
}

static int cmd_audio_role_get(const struct shell *shell, size_t argc,
                           char **argv)
{
	if (argc == 2) {
		shell_error(shell, "%s:%s%s", argv[0],
			    SHELL_MSG_UNKNOWN_PARAMETER, argv[1]);
		return -EINVAL;
	}

	shell_print(shell, "Role is: %s",
			(audio_get_role() == player) ? "player" : "recorder");

	return 0;
}

static int cmd_audio_role_set_player(const struct shell *shell, size_t argc,
		char **argv)
{
	audio_set_role(player);
	shell_print(shell, "Role is: %s", "player");
	return 0;
}

static int cmd_audio_role_set_recorder(const struct shell *shell, size_t argc,
		char **argv)
{
	audio_set_role(recorder);
	shell_print(shell, "Role is: %s", "recorder");
	return 0;
}

static int cmd_audio_volume(const struct shell *shell, size_t argc,
        char **argv)
{
	uint32_t volume;
    int ret;

//    if(argc == 1){
//    	volume = audio_volume_get();
//    }

    if(argc == 2){
    	ret = sscanf(argv[1], "%u", &volume);
    	if(ret != 1){
    		shell_error(shell, "%s:%s%s", argv[0],
    				SHELL_MSG_UNKNOWN_PARAMETER, argv[1]);
    		return -EINVAL;
    	}


    	SGTL5000_volumeInteger(volume);
    }

	shell_print(shell, "volume is %d / %d", volume, SGTL5000_volumeInteger_get_max());
	return 0;
}


static int cmd_audio_input_Mic(const struct shell *shell, size_t argc,
        char **argv)
{
    SGTL5000_inputSelect(AUDIO_INPUT_MIC);
	shell_print(shell, "Input: Mic");

	return 0;
}

static int cmd_audio_input_LineIn(const struct shell *shell, size_t argc,
        char **argv)
{
    SGTL5000_inputSelect(AUDIO_INPUT_LINEIN);
	shell_print(shell, "Input: LineIN");

    return 0;
}

static int cmd_audio_gain_Mic(const struct shell *shell, size_t argc,
        char **argv)
{
	uint32_t dB;
    int ret;

//    if(argc == 1){
//    	volume = audio_volume_get();
//    }

    if(argc == 2){
    	ret = sscanf(argv[1], "%u", &dB);
    	if(ret != 1){
    		shell_error(shell, "%s:%s%s", argv[0],
    				SHELL_MSG_UNKNOWN_PARAMETER, argv[1]);
    		return -EINVAL;
    	}

    	SGTL5000_micGain(dB);
    	shell_print(shell, "mic gain level:\t %d dB", dB);
    }

	return 0;
}

static int cmd_audio_gain_LineIn(const struct shell *shell, size_t argc,
        char **argv)
{
	uint32_t level;
    int ret;

//    if(argc == 1){
//    	volume = audio_volume_get();
//    }

    if(argc == 2){
    	ret = sscanf(argv[1], "%u", &level);
    	if(ret != 1){
    		shell_error(shell, "%s:%s%s", argv[0],
    				SHELL_MSG_UNKNOWN_PARAMETER, argv[1]);
    		return -EINVAL;
    	}
    	SGTL5000_lineInLevel((uint8_t) level);
    	shell_print(shell, "lineIn gain level:\t %d/15", level);
    }

	return 0;
}


SHELL_STATIC_SUBCMD_SET_CREATE(m_sub_mute_set,
	SHELL_CMD_ARG(on, NULL, "Mute On", cmd_audio_mute, 1, 0),
	SHELL_CMD_ARG(off, NULL, "Mute Off", cmd_audio_unmute, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(m_sub_role_set,
	SHELL_CMD_ARG(player, NULL, "Configure as a Player.", cmd_audio_role_set_player, 1, 0),
	SHELL_CMD_ARG(recorder, NULL, "Configure as a Recorder.", cmd_audio_role_set_recorder, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(m_sub_input,
	SHELL_CMD_ARG(mic, NULL, "Select Mic.", cmd_audio_input_Mic, 1, 0),
	SHELL_CMD_ARG(linein, NULL, "Select LineIn.", cmd_audio_input_LineIn, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(m_sub_gain,
	SHELL_CMD_ARG(mic, NULL, "gain mic [0:63dB].", cmd_audio_gain_Mic, 2, 0),
	SHELL_CMD_ARG(linein, NULL, "gain LineIn [0:15]*1.5dB.", cmd_audio_gain_LineIn, 2, 0),
	SHELL_SUBCMD_SET_END
);




SHELL_STATIC_SUBCMD_SET_CREATE(m_sub_audio,
	SHELL_CMD_ARG(mute, &m_sub_mute_set, "mute or unmute.", cmd_audio_mute_get, 1, 1),
	SHELL_CMD_ARG(role, &m_sub_role_set, "Audio role.", cmd_audio_role_get, 1, 1),
	SHELL_CMD_ARG(volume, NULL, "Get or set volume", cmd_audio_volume, 2, 0),
	SHELL_CMD_ARG(input, &m_sub_input, "Select audio input.", NULL, 2, 0),
	SHELL_CMD_ARG(gain, &m_sub_gain, "Select gain level.", NULL, 3, 0),

	SHELL_SUBCMD_SET_END
);


/* Creating root (level 0) command "demo" without a handler */
SHELL_CMD_REGISTER(audio, &m_sub_audio, "Audio commands.", NULL);

