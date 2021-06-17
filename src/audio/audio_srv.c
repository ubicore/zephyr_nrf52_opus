/* echo.c - Networking echo server */

/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(audio, LOG_LEVEL_DBG);

#include <zephyr.h>
#include <stdio.h>
#include <sys/types.h>

#include <linker/sections.h>
#include <errno.h>

#include "config.h"
#include "codec_opus.h"
#include "codec_I2S.h"

#include "user_led.h"

#include "radio.h"

//#define I2S_LOOPBACK

#define OPUS_PRIORITY 10

#define OPUS_ENCODE_STACK_SIZE 20000
#define OPUS_DECODE_STACK_SIZE 50000

K_THREAD_STACK_DEFINE(Audio_Rx_stack_area, OPUS_ENCODE_STACK_SIZE);
struct k_thread Audio_Rx_thread_data;
K_THREAD_STACK_DEFINE(Audio_Tx_stack_area, OPUS_DECODE_STACK_SIZE);
struct k_thread Audio_Tx_thread_data;

k_tid_t Audio_Rx_tid;
k_tid_t Audio_Tx_tid;

audio_stats_counter_t audio_stats_counter;

static PCM_Rx_t Data_In;
static PCM_Tx_t Data_Out;
static PCM_Rx_t * p_Data_In = &Data_In;
static PCM_Tx_t * p_Data_Out = &Data_Out;

static m_audio_rec_frame_t frame_OPUS_encode;
static m_audio_play_frame_t frame_OPUS_decode;


#define Q_OPUS_OUT_MAX_MSGS 2 // Opus encoder -> radio : > 1 to be able to use LBT without frame lost.
#define Q_OPUS_IN_MAX_MSGS 2 // radio -> Opus decoder :  > 1 to be able to use LBT without frame lost. each buffer add a frame duration latency
#define Q_OPUS_IN_uSD_MAX_MSGS 20 // TODO : check the SPI bus frequency using the correct SPI peripheral !!!
#define Q_RX_OUT_MAX_MSGS 1 // I2S -> Opus encoder
#define Q_TX_OUT_MAX_MSGS 2 // Opus decoder -> I2S

K_MSGQ_DEFINE(msgq_OPUS_OUT , sizeof(m_audio_rec_frame_t), Q_OPUS_OUT_MAX_MSGS, 4);
struct k_msgq *p_msgq_OPUS_OUT = &msgq_OPUS_OUT ;
K_MSGQ_DEFINE(msgq_OPUS_IN , sizeof(m_audio_play_frame_t), Q_OPUS_IN_MAX_MSGS, 4);
struct k_msgq *p_msgq_OPUS_IN = &msgq_OPUS_IN ;

K_MSGQ_DEFINE(msgq_OPUS_IN_FromuSD , sizeof(m_audio_play_frame_t), Q_OPUS_IN_uSD_MAX_MSGS, 4);
struct k_msgq *p_msgq_OPUS_IN_FromuSD = &msgq_OPUS_IN_FromuSD ;


K_MSGQ_DEFINE(msgq_Rx_IN, sizeof(PCM_Rx_t), Q_RX_OUT_MAX_MSGS, 4);
struct k_msgq *p_msgq_Rx_IN = &msgq_Rx_IN;
K_MSGQ_DEFINE(msgq_Tx_OUT, sizeof(PCM_Tx_t), Q_TX_OUT_MAX_MSGS, 4);
struct k_msgq *p_msgq_Tx_OUT = &msgq_Tx_OUT;


struct audio_parameters_t
{
	audio_role_t role; /*Default role is player*/
	bool mute;

};

struct audio_parameters_t audio;

void audio_mute_set(bool mute)
{
	audio.mute = mute;
}

bool audio_mute_get()
{
	return audio.mute;
}

void audio_set_role(audio_role_t new_role)
{
	//set radio role according to audio
	if(new_role == recorder){
		radio_mode_select(MODE_BROADCASTER);
	}else{
		radio_mode_select(MODE_RECEIVER);
	}

	//if(audio_role == new_role) return;
	LOG_INF("new_role -> %s", (new_role == player) ? "player" : "recorder");
	audio.role = new_role;
}

audio_role_t audio_get_role()
{
	return audio.role;
}

void audio_stats_print(){
	audio_stats_counter_t stats;

	memcpy(&stats, &audio_stats_counter, sizeof(audio_stats_counter));

	//Reset Stat
	memset(&audio_stats_counter, 0, sizeof(audio_stats_counter));
	LOG_INF("--------------------------");
	LOG_INF("------ Audio Stats -------");
	LOG_INF("--------------------------");

	if(stats.Opus_enc)LOG_INF("Enc:\t %u", stats.Opus_enc);
	if(stats.I2S_Rx_discared)LOG_ERR("I2S_Rx_discared:\t %u", stats.I2S_Rx_discared);
	if(stats.Opus_dec)
		LOG_INF("Dec:\t %u", stats.Opus_dec);
	if(stats.Opus_gen)
		LOG_WRN("Opus_gen:\t %u", stats.Opus_gen);
	if(stats.Opus_gen_empty)
		LOG_WRN("Opus_gen_empty:\t %u", stats.Opus_gen_empty);
	if(stats.I2S_Tx_missing)
		LOG_ERR("I2S_Tx_missing:\t %u", stats.I2S_Tx_missing);

	if(audio_get_role() == player){
		LOG_INF("Dec take:\t %u < * < %u ms", stats.Opus_dec_spent_min, stats.Opus_dec_spent_max);
		LOG_INF("Gen take:\t %u < * < %u ms", stats.Opus_gen_spent_min, stats.Opus_gen_spent_max);
	}else{

	}

	LOG_INF("");
}


#ifndef CONFIG_AUDIO_CODEC_SGTL5000

static struct k_sem Rx_lock;
static void Rx_unlock(struct k_timer *timer_id);
K_TIMER_DEFINE(Rx_timer, Rx_unlock, NULL);

static void Rx_unlock(struct k_timer *timer_id)
{
	k_sem_give(&Rx_lock);
}
#endif

static void Audio_Rec_Process()
{
	m_audio_rec_frame_t *p_frame =  &frame_OPUS_encode;

#if CONFIG_AUDIO_CODEC_SGTL5000
	k_msgq_get(p_msgq_Rx_IN, p_Data_In, K_FOREVER);
#else
	k_sem_take(&Rx_lock, K_FOREVER);
#endif

	//
	audio_stats_counter.Opus_enc++;
	/*copy timestamp*/
	p_frame->timestamp_rec.I2S_Out = p_Data_In->timestamp_I2S_Out;
	/*encode pcm to opus*/
	Led_Set(led_encode);
	/*capture time stamp*/
	p_frame->timestamp_rec.Enc_In = k_uptime_get_32();
	drv_audio_codec_encode(p_Data_In->buff, &p_frame->opus_frame);
	/*capture time stamp*/
	p_frame->timestamp_rec.Enc_Out = k_uptime_get_32();
	Led_Clear(led_encode);

	/**/
	if(audio.mute){
		/*do not send any audio packet to radio*/
		return;
	}
	k_msgq_put(p_msgq_OPUS_OUT, p_frame, K_FOREVER);
}



void audio_stats_update_spent_to_dec(s64_t milliseconds_spent){

	if(milliseconds_spent > audio_stats_counter.Opus_dec_spent_max){
		audio_stats_counter.Opus_dec_spent_max = (uint32_t) milliseconds_spent;
	}

	if((milliseconds_spent < audio_stats_counter.Opus_dec_spent_min) || (audio_stats_counter.Opus_dec_spent_min == 0)){
		audio_stats_counter.Opus_dec_spent_min = (uint32_t) milliseconds_spent;
	}
}

void audio_stats_update_spent_to_gen(s64_t milliseconds_spent){

	if(milliseconds_spent > audio_stats_counter.Opus_gen_spent_max){
		audio_stats_counter.Opus_gen_spent_max = (uint32_t) milliseconds_spent;
	}

	if((milliseconds_spent < audio_stats_counter.Opus_gen_spent_min) || (audio_stats_counter.Opus_gen_spent_min == 0)){
		audio_stats_counter.Opus_gen_spent_min = (uint32_t) milliseconds_spent;
	}
}


static void Audio_Play_Process()
{
	m_audio_play_frame_t *p_frame = &frame_OPUS_decode;

	if(audio.mute == false){
		/*Decode from radio*/
		if(k_msgq_get(p_msgq_OPUS_IN, p_frame, K_NO_WAIT) == 0)
		{
			/*copy timestamp*/
			p_Data_Out->timestamp_play.radio_Out = p_frame->timestamp_radio_Out;
			Led_Set(led_decode);
			/*capture time stamp*/
			p_Data_Out->timestamp_play.Dec_In = k_uptime_get_32();
			/*decode opus to pcm*/
			drv_audio_codec_decode(&p_frame->opus_frame, p_Data_Out->buff);
			/*capture time stamp*/
			p_Data_Out->timestamp_play.Dec_Out = k_uptime_get_32();
			Led_Clear(led_decode);
			audio_stats_counter.Opus_dec++;
			/*Send msg to I2S*/
			k_msgq_put(p_msgq_Tx_OUT, p_Data_Out, K_FOREVER);
			return;
		}

		/*Decode from uSD*/
		if(k_msgq_get(p_msgq_OPUS_IN_FromuSD, p_frame, K_NO_WAIT) == 0)
		{
			Led_Set(led_decode);
			/*capture time stamp*/
			p_Data_Out->timestamp_play.Dec_In = k_uptime_get_32();
			/*decode opus to pcm*/
			drv_audio_codec_decode(&p_frame->opus_frame, p_Data_Out->buff);
			/*capture time stamp*/
			p_Data_Out->timestamp_play.Dec_Out = k_uptime_get_32();
			Led_Clear(led_decode);
			audio_stats_counter.Opus_dec++;
			/*Send msg to I2S*/
			k_msgq_put(p_msgq_Tx_OUT, p_Data_Out, K_FOREVER);
			return;
		}
	}

	p_frame->opus_frame.data_size = 0;
	Led_Set(led_gen);
	/*capture time stamp*/
	p_Data_Out->timestamp_play.Dec_In = k_uptime_get_32();
	/*generate intermediate frame*/
	drv_audio_codec_decode(&p_frame->opus_frame, p_Data_Out->buff);
	/*capture time stamp*/
	p_Data_Out->timestamp_play.Dec_Out = k_uptime_get_32();
	Led_Clear(led_gen);
	audio_stats_counter.Opus_gen++;

	/*Send msg to I2S*/
	k_msgq_put(p_msgq_Tx_OUT, p_Data_Out, K_FOREVER);
}

static void Audio_Rx_Task(){

	LOG_INF("Rx_Task running");
#ifndef CONFIG_AUDIO_CODEC_SGTL5000
	k_sem_init(&Rx_lock, 0, UINT_MAX);
	k_timer_start(&Rx_timer, CONFIG_AUDIO_FRAME_SIZE_MS, CONFIG_AUDIO_FRAME_SIZE_MS);
#endif
	while(1){
		Audio_Rec_Process();
	}
}

static void Audio_Tx_Task(){

	LOG_INF("Tx_Task running");
	while(1){
		Audio_Play_Process();
	}
}


void audio_server_init()
{
	audio_codec_opus_init();

	audio.role = player;

	Audio_Rx_tid = k_thread_create(&Audio_Rx_thread_data, Audio_Rx_stack_area,
	                                 K_THREAD_STACK_SIZEOF(Audio_Rx_stack_area),
									 Audio_Rx_Task,
	                                 NULL, NULL, NULL,
									 OPUS_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&Audio_Rx_thread_data, "Audio_Rx");

	Audio_Tx_tid = k_thread_create(&Audio_Tx_thread_data, Audio_Tx_stack_area,
	                                 K_THREAD_STACK_SIZEOF(Audio_Tx_stack_area),
									 Audio_Tx_Task,
	                                 NULL, NULL, NULL,
									 OPUS_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&Audio_Tx_thread_data, "Audio_Tx");

	k_sleep(K_MSEC(1000));
#if CONFIG_AUDIO_CODEC_SGTL5000
	audio_codec_I2S_init();
	audio_codec_I2S_connect(p_msgq_Rx_IN, p_msgq_Tx_OUT);
#ifdef I2S_LOOPBACK
	audio_codec_I2S_start_loopback();
#else
	audio_codec_I2S_start();
#endif
#endif
}

void audio_server_stop(){


}

