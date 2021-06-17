/*
 * audio_server.h
 *
 *  Created on: 14 mars 2019
 *      Author: nlantz
 */

#ifndef SAMPLES_AUDIO_NET_OPUS_RADIO_SRC_AUDIO_AUDIO_SERVER_H_
#define SAMPLES_AUDIO_NET_OPUS_RADIO_SRC_AUDIO_AUDIO_SERVER_H_

typedef struct {
	uint32_t I2S_Rx_discared;
	uint32_t I2S_Tx_missing;
	uint32_t Opus_enc;
	uint32_t Opus_dec;
	uint32_t Opus_gen;
	uint32_t Opus_gen_empty;
	uint32_t Opus_dec_spent_max;
	uint32_t Opus_dec_spent_min;
	uint32_t Opus_gen_spent_max;
	uint32_t Opus_gen_spent_min;

	uint32_t Opus_declost_spent_max;
	uint32_t Opus_declost_spent_min;
}audio_stats_counter_t;

typedef enum
{
	recorder,
	player,
} audio_role_t;


typedef struct
{
    uint16_t    data_size;
	uint8_t     data[OPUS_AUDIO_FRAME_SIZE_BYTES];
}opus_frame_t;

typedef struct
{
	s32_t timestamp_radio_Out;
	opus_frame_t opus_frame;
}__attribute__((packed)) m_audio_play_frame_t;

typedef struct
{
	timestamp_recorder_t timestamp_rec;
	opus_frame_t opus_frame;
}__attribute__((packed)) m_audio_rec_frame_t;

extern struct audio_parameters_t audio;

void audio_mute_set(bool mute);
bool audio_mute_get();
void audio_set_role(audio_role_t new_role);
audio_role_t audio_get_role();
void audio_server_init();
void audio_server_stop();
void audio_stats_print();



#endif /* SAMPLES_AUDIO_NET_OPUS_RADIO_SRC_AUDIO_AUDIO_SERVER_H_ */
