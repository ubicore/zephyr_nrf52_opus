/*
 * latency.h
 *
 *  Created on: 7 juin 2019
 *      Author: nlantz
 */

#ifndef SRC_AUDIO_LATENCY_H_
#define SRC_AUDIO_LATENCY_H_


#include "config.h"

typedef struct
{
	s32_t  I2S_Out;
	s32_t  Enc_In;
	s32_t  Enc_Out;
	s32_t  radio_In;
	s32_t  radio_End;
}timestamp_recorder_t;

typedef struct
{
	s32_t radio_Out;
	s32_t Dec_In;
	s32_t Dec_Out;
	s32_t I2S_In;
}timestamp_player_t;

//
//	s64_t  reftime = p_PCM_Tx->timestamp_play.I2S_In;
///* compute how long the work took (also updates the time stamp) */
//p_latency->player = k_uptime_delta_32(&reftime);
//


typedef struct
{
	timestamp_recorder_t  recorder;
	timestamp_player_t  player;
}timestamp_t;



typedef struct {
	s32_t  enc;
	s32_t  full;
}latency_recorder_t;

typedef struct {
	s32_t  dec;
	s32_t  full;
}latency_player_t;

typedef struct {
	latency_recorder_t  recorder;
	latency_player_t  player;
}latency_t;


void latency_stats_print();
void latency_recorder_update(timestamp_recorder_t * p_timestamp);
void latency_recorder_set(s32_t latency);
s32_t latency_recorder_get();
void latency_player_update(timestamp_player_t * p_timestamp);


#define I2S_LATENCY CONFIG_AUDIO_FRAME_SIZE_MS


#endif /* SRC_AUDIO_LATENCY_H_ */
