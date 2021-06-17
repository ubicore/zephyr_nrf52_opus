/*
 * latency.c
 *
 *  Created on: 7 juin 2019
 *      Author: nlantz
 */


#include <logging/log.h>
LOG_MODULE_DECLARE(audio, LOG_LEVEL_DBG);

#include <stdio.h>
#include <string.h>
#include <zephyr.h>
#include "config.h"

static timestamp_t  timestamp;
static latency_t  latency;

timestamp_t * p_timestamp = &timestamp;
static latency_t * p_latency = &latency;


void latency_stats_print(){

	LOG_INF("--------------------------");
	LOG_INF("----- Latency Stats ------");
	LOG_INF("--------------------------");

	if(audio_get_role() == recorder){
		LOG_INF("recorder.enc:\t %d", p_latency->recorder.enc);
	}

	LOG_INF("recorder.full:\t %d", p_latency->recorder.full );

	if(audio_get_role() == player){
		LOG_INF("player.dec:\t %d", p_latency->player.dec );
		LOG_INF("player.full:\t %d", p_latency->player.full);

		s32_t sum = p_latency->recorder.full + p_latency->player.full;
		LOG_INF("SUM:\t %d",sum);
	}
	LOG_INF("");
}

void latency_recorder_update(timestamp_recorder_t * p_timestamp)
{
	/*compute how long the work took to encode audio*/
	p_latency->recorder.enc = p_timestamp->Enc_Out - p_timestamp->Enc_In;
	/*compute how long the work took from sampling to radio end*/
	p_latency->recorder.full = I2S_LATENCY + p_timestamp->radio_End - p_timestamp->I2S_Out;
}

void latency_recorder_set(s32_t latency)
{
	p_latency->recorder.full = latency;
}

s32_t latency_recorder_get()
{
	return p_latency->recorder.full;
}

void latency_player_update(timestamp_player_t * p_timestamp)
{
	/*compute how long the work took to decode audio*/
	p_latency->player.dec = p_timestamp->Dec_Out - p_timestamp->Dec_In;
	/*compute how long the work took from sampling to play*/
	p_latency->player.full =  I2S_LATENCY + p_timestamp->I2S_In - p_timestamp->radio_Out;
}






