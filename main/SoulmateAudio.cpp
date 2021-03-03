#include "SoulmateAudio.h"

#define CONFIG_I2S_BCK_PIN 19
#define CONFIG_I2S_LRCK_PIN 33
#define CONFIG_I2S_DATA_PIN 22
#define CONFIG_I2S_DATA_IN_PIN 23

#define SPEAKER_I2S_NUMBER I2S_NUM_0

#define MIC_SAMPLE_FREQUENCY    16000   // Low samplerates not possible due to hardware requirements
#define NUM_WINDOWS_PER_SECOND  50      // 20ms per window
#define NUM_BUF_WINDOWS         2       // store N windows of audio

typedef struct {
    uint16_t offset;
    uint16_t window_length;
    uint8_t n_windows;
    uint8_t n_windows_available;
    int16_t *samples;
} audio_buffer_t;

static audio_buffer_t audioBuf;

esp_err_t InitI2SAudio(audio_mode_t mode)
{
    esp_err_t err = i2s_driver_uninstall(SPEAKER_I2S_NUMBER);
    if (err != ESP_OK) {
        return err;
    }

    // setup audio buffer
    audioBuf.offset = 0;
    audioBuf.window_length = MIC_SAMPLE_FREQUENCY / NUM_WINDOWS_PER_SECOND; // 20ms
    audioBuf.n_windows = NUM_BUF_WINDOWS;
    audioBuf.n_windows_available = 0;
    uint16_t buf_size = audioBuf.window_length * audioBuf.n_windows * sizeof(audioBuf.samples[0]);
    if (audioBuf.samples != NULL) {
        free(audioBuf.samples);
    }
    audioBuf.samples = (int16_t*)malloc(buf_size);
    if (audioBuf.samples == NULL) {
        return ESP_FAIL;
    }
    
    // configure i2s
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER),
        .sample_rate = MIC_SAMPLE_FREQUENCY,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT, // is fixed at 12bit, stereo, MSB
        .channel_format = I2S_CHANNEL_FMT_ALL_RIGHT,
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = audioBuf.n_windows,
        .dma_buf_len = audioBuf.window_length,
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

static void audioUpdate(void)
{
    // Find out how many bytes to read
    uint16_t buf_len = audioBuf.window_length * audioBuf.n_windows;
    uint16_t n_free_samples = buf_len - audioBuf.offset;
    uint16_t n_bytes_to_read = (n_free_samples < audioBuf.window_length) ?
        (n_free_samples * sizeof(audioBuf.samples[0])) :
        (audioBuf.window_length * sizeof(audioBuf.samples[0]));

    // read bytes
    size_t bytes_read;
    i2s_read(SPEAKER_I2S_NUMBER, (audioBuf.samples + audioBuf.offset), n_bytes_to_read, &bytes_read, 0);
    audioBuf.offset = (audioBuf.offset + bytes_read) % buf_len;

    // update n_windows_available
    if (bytes_read > 0) {
        Serial.printf("NumSaples: %d\n", bytes_read/2);
        if (audioBuf.offset % audioBuf.window_length == 0) {    // complete window read
            audioBuf.n_windows_available++;
        } else if (audioBuf.n_windows_available == audioBuf.n_windows_available) { // partial window read with buffer overrun
            audioBuf.n_windows_available--;
        }

        // complete window read with buffer overrun
        if (audioBuf.n_windows_available > audioBuf.n_windows_available) {
            audioBuf.n_windows_available = audioBuf.n_windows;
        }
    }
}

uint16_t audioAvailable(void) {
    return audioBuf.n_windows_available * audioBuf.window_length;
}
