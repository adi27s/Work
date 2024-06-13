#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "audio_common.h"
#include "audio_sys.h"
#include "board.h"
#include "i2s_stream.h"
#include "nvs_flash.h"

static const char *TAG = "MIC_TO_SPEAKER";

typedef enum {
    AUDIO_DATA_FORMNAT_ONLY_RIGHT,
    AUDIO_DATA_FORMNAT_ONLY_LEFT,
    AUDIO_DATA_FORMNAT_RIGHT_LEFT,
} audio_channel_format_t;

static esp_err_t audio_data_format_set(i2s_stream_cfg_t *i2s_cfg, audio_channel_format_t fmt)
{
    AUDIO_UNUSED(i2s_cfg);   // remove unused warning
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    switch (fmt) {
        case AUDIO_DATA_FORMNAT_ONLY_RIGHT:
            i2s_cfg->std_cfg.slot_cfg.slot_mode = I2S_SLOT_MODE_MONO;
            i2s_cfg->std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_RIGHT;
            break;
        case AUDIO_DATA_FORMNAT_ONLY_LEFT:
            i2s_cfg->std_cfg.slot_cfg.slot_mode = I2S_SLOT_MODE_MONO;
            i2s_cfg->std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;
            break;
        case AUDIO_DATA_FORMNAT_RIGHT_LEFT:
            i2s_cfg->std_cfg.slot_cfg.slot_mode = I2S_SLOT_MODE_STEREO;
            i2s_cfg->std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_BOTH;
            break;
    }
#else
    switch (fmt) {
        case AUDIO_DATA_FORMNAT_ONLY_RIGHT:
            i2s_cfg->i2s_config.channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT;
            break;
        case AUDIO_DATA_FORMNAT_ONLY_LEFT:
            i2s_cfg->i2s_config.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
            break;
        case AUDIO_DATA_FORMNAT_RIGHT_LEFT:
            i2s_cfg->i2s_config.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
            break;
    }
#endif
    return ESP_OK;
}

void listen_from_mic(void)
{
    audio_pipeline_handle_t pipeline;
    audio_element_handle_t i2s_stream_reader, i2s_stream_writer;
    int channel_format = AUDIO_DATA_FORMNAT_RIGHT_LEFT;
    int sample_rate = 16000;

    esp_log_level_set("*", ESP_LOG_WARN);
    esp_log_level_set(TAG, ESP_LOG_INFO);

    ESP_LOGI(TAG, "[ 1 ] Start codec chip");
    audio_board_handle_t board_handle = audio_board_init();
    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_BOTH, AUDIO_HAL_CTRL_START);

    ESP_LOGI(TAG, "[2.0] Create audio pipeline for recording");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);
    mem_assert(pipeline);

    ESP_LOGI(TAG, "[2.1] Create i2s stream to read audio data from codec chip");
    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg.type = AUDIO_STREAM_READER;

    audio_data_format_set(&i2s_cfg, channel_format);
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    i2s_cfg.std_cfg.clk_cfg.sample_rate_hz = sample_rate;
#else
    i2s_cfg.i2s_config.sample_rate = sample_rate;
#endif
    i2s_stream_reader = i2s_stream_init(&i2s_cfg);

    i2s_cfg.i2s_port = I2S_NUM_0;
    i2s_cfg.type = AUDIO_STREAM_WRITER;
    i2s_stream_writer = i2s_stream_init(&i2s_cfg);

    ESP_LOGI(TAG, "[2.2] Register all elements to audio pipeline");
    audio_pipeline_register(pipeline, i2s_stream_reader, "i2s_reader");
    audio_pipeline_register(pipeline, i2s_stream_writer, "i2s_writer");

    ESP_LOGI(TAG, "[2.3] Link elements together [codec_chip]-->i2s_stream_reader-->i2s_stream_writer-->[speaker]");
    const char *link_tag[2] = {"i2s_reader", "i2s_writer"};
    audio_pipeline_link(pipeline, &link_tag[0], 2);

    ESP_LOGI(TAG, "[ 3 ] Set up event listener");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);

    ESP_LOGI(TAG, "[3.1] Listening event from pipeline");
    audio_pipeline_set_listener(pipeline, evt);

    ESP_LOGI(TAG, "[ 4 ] Start audio_pipeline");
    audio_pipeline_run(pipeline);

    ESP_LOGI(TAG, "[ 5 ] Listen for all pipeline events");
    while (1) {
        audio_event_iface_msg_t msg;
        if (audio_event_iface_listen(evt, &msg, 1000 / portTICK_RATE_MS) == ESP_OK) {
            if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *) i2s_stream_reader
                && msg.cmd == AEL_MSG_CMD_REPORT_STATUS) {
                ESP_LOGI(TAG, "[ * ] Receiving audio data");
            }
        }
    }

    ESP_LOGI(TAG, "[ 6 ] Stop audio_pipeline");
    audio_pipeline_stop(pipeline);
    audio_pipeline_wait_for_stop(pipeline);
    audio_pipeline_terminate(pipeline);

    audio_pipeline_unregister(pipeline, i2s_stream_reader);
    audio_pipeline_unregister(pipeline, i2s_stream_writer);

    audio_pipeline_remove_listener(pipeline);

    audio_event_iface_destroy(evt);

    audio_pipeline_deinit(pipeline);
    audio_element_deinit(i2s_stream_reader);
    audio_element_deinit(i2s_stream_writer);
}

void app_main(void)
{
    listen_from_mic();
}
