#include "SoulmateAudio.h"

#define CONFIG_I2S_BCK_PIN 19
#define CONFIG_I2S_LRCK_PIN 33
#define CONFIG_I2S_DATA_PIN 22
#define CONFIG_I2S_DATA_IN_PIN 23

#define SPEAKER_I2S_NUMBER I2S_NUM_0

#define MIC_SAMPLE_FREQUENCY    16000   // Low samplerates not possible due to hardware requirements
#define NUM_WINDOWS_PER_SECOND  50      // 20ms per window
#define NUM_BUF_WINDOWS         2       // store N windows of audio

#define MICROSECONDS_PER_S      (1000000)   
#define WINDOW_PERIOD_US        (MICROSECONDS_PER_S / NUM_WINDOWS_PER_SECOND)  // Microseconds per sampling window

static int16_t audioSamples[MIC_SAMPLE_FREQUENCY / NUM_WINDOWS_PER_SECOND];
static audio_frame_t audioFrame = {
    .frame_number = 0,
    .sample_frequency = MIC_SAMPLE_FREQUENCY,
    .num_samples = 0,
    .samples = audioSamples,
};

// Disable all audio
esp_err_t audioDisable(void)
{
    return audioDisableI2S();
}

esp_err_t audioEnableI2S(audio_mode_t mode)
{
    esp_err_t err = i2s_driver_uninstall(SPEAKER_I2S_NUMBER);
    if (err != ESP_OK) {
        return err;
    }

    // setup audio frame
    audioFrame.frame_number = 0;
    
    // configure i2s
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER),
        .sample_rate = MIC_SAMPLE_FREQUENCY,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT, // is fixed at 12bit, stereo, MSB
        .channel_format = I2S_CHANNEL_FMT_ALL_RIGHT,
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = NUM_BUF_WINDOWS,
        .dma_buf_len = MIC_SAMPLE_FREQUENCY / NUM_WINDOWS_PER_SECOND,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0,
    };

    // Set i2s mode
    if (mode == MODE_MIC) {
        i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM);
    } else {
        i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
        i2s_config.use_apll = false;
        i2s_config.tx_desc_auto_clear = true;
    }

    err = i2s_driver_install(SPEAKER_I2S_NUMBER, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        return err;
    }

    i2s_pin_config_t tx_pin_config;
    tx_pin_config.bck_io_num = CONFIG_I2S_BCK_PIN;
    tx_pin_config.ws_io_num = CONFIG_I2S_LRCK_PIN;
    tx_pin_config.data_out_num = CONFIG_I2S_DATA_PIN;
    tx_pin_config.data_in_num = CONFIG_I2S_DATA_IN_PIN;

    err += i2s_set_pin(SPEAKER_I2S_NUMBER, &tx_pin_config);
    err += i2s_set_clk(SPEAKER_I2S_NUMBER, MIC_SAMPLE_FREQUENCY, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);

    return err;
}

esp_err_t audioDisableI2S(void)
{
    return i2s_driver_uninstall(SPEAKER_I2S_NUMBER);
}

static esp_err_t audioUpdateI2S(void)
{
    // check time since last update
    static int64_t last_update_time_us;
    int64_t current_time_us = esp_timer_get_time();
    if ((current_time_us - last_update_time_us) < (WINDOW_PERIOD_US + 250)) {
        return ESP_OK;
    }
    last_update_time_us = current_time_us;

    // read bytes
    size_t bytes_read;
    esp_err_t status = i2s_read(SPEAKER_I2S_NUMBER, audioSamples, sizeof(audioSamples), &bytes_read, 0);
    audioFrame.num_samples = bytes_read >> 1; // bytes_read >> log2(sizeof(audioSamples[0]))
    audioFrame.frame_number++;

    return status;
}

audio_frame_t* audioGetFrameI2S(void) {
    esp_err_t status = ESP_FAIL;

#ifdef USE_MICROPHONE
    status = audioUpdateI2S();
#endif

    if (status != ESP_OK) {
        return NULL;
    }

    return &audioFrame;
}
