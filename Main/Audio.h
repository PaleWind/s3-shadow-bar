//#include <fix_fft.h>

/*
 * This code has been taken from the sound reactive fork of WLED by Andrew
 * Tuline, and the analog audio processing has been (mostly) removed as we
 * will only be using the INMP441.
 * 
 * The FFT code runs on core 0 while everything else runs on core 1. This
 * means we can make our main code more complex without affecting the FFT
 * processing.
 */

#include <driver/i2s.h>
#include "arduinoFFT.h"

#define I2S_WS 6        // aka LRCL
#define I2S_SD 4        // aka DOUT
#define I2S_SCK 5       // aka BCLK
#define MIN_SHOW_DELAY  15
const i2s_port_t I2S_PORT = I2S_NUM_0;
const int BLOCK_SIZE = 64;

const int SAMPLE_RATE = 10240;

TaskHandle_t FFT_Task;

uint16_t micData;                               // Analog input for FFT
uint16_t micDataSm;                             // Smoothed mic data, as it's a bit twitchy

const uint16_t samples = 512;                     // This value MUST ALWAYS be a power of 2
unsigned int sampling_period_us;
unsigned long microseconds;

double FFT_MajorPeak = 0;
double FFT_Magnitude = 0;
uint16_t mAvg = 0;

// These are the input and output vectors.  Input vectors receive computed results from FFT.
double vReal[samples];
double vImag[samples];
double fftBin[samples];

// Try and normalize fftBin values to a max of 4096, so that 4096/16 = 256.
// Oh, and bins 0,1,2 are no good, so we'll zero them out.
double fftCalc[16];
int fftResult[16];                      // Our calculated result table, which we feed to the animations.
double fftResultMax[16];                // A table used for testing to determine how our post-processing is working.

// Table of linearNoise results to be multiplied by squelch in order to reduce squelch across fftResult bins.
//int linearNoise[16] = { 34, 28, 26, 25, 20, 12, 9, 6, 4, 4, 3, 2, 2, 2, 2, 2 };
int linearNoise[16] = { 40, 34, 32, 30, 24, 16, 9, 6, 3, 3, 2, 1, 1, 1, 1, 1 };

// Table of multiplication factors so that we can even out the frequency response.
//double fftResultPink[16] = {1.70,1.71,1.73,1.78,1.68,1.56,1.55,1.63,1.79,1.62,1.80,2.06,2.47,3.35,6.83,9.55};
double fftResultPink[16] = {.93,.97,1.0,1.02,1.68,1.56,1.55,1.63,1.79,2,2.80,3.56,4,5,9,11};


// Create FFT object
arduinoFFT FFT = arduinoFFT( vReal, vImag, samples, SAMPLE_RATE );

void fillFFT()
{
  for (int i = 0; i < 16; i++) 
  {
    uint8_t fftValue = fftResult[i];
    fftValue = ((prevFFTValue[i] * 3) + fftValue) / 4;            // Dirty rolling average between frames to reduce flicker
    barHeights[i] = fftValue / (255 / M_HEIGHT);                  // Scale bar height
    prevFFTValue[i] = fftValue;                                   // Save prevFFTValue for averaging later
    //Serial.print(i);Serial.print(' ');Serial.println(barHeights[i]);
//    Serial.print(20);Serial.print(' '); 
//    Serial.println(0);
  }
}

double fftAdd( int from, int to) {
  int i = from;
  double result = 0;
  while ( i <= to) {
    result += fftBin[i++];
  }
  return result;
}

// FFT main code
void FFTcode( void * parameter) {

  for(;;) {
    delay(1);           // DO NOT DELETE THIS LINE! It is needed to give the IDLE(0) task enough time and to keep the watchdog happy.
                        // taskYIELD(), yield(), vTaskDelay() and esp_task_wdt_feed() didn't seem to work.
                        
    microseconds = micros();

    for(int i=0; i<samples; i++) {
      int32_t digitalSample = 0;
      size_t bytes_read = 0;
      esp_err_t result = i2s_read(I2S_PORT, &digitalSample, sizeof(digitalSample), &bytes_read, /*portMAX_DELAY*/ 10);
      //int bytes_read = i2s_pop_sample(I2S_PORT, (char *)&digitalSample, portMAX_DELAY); // no timeout
      if (bytes_read > 0) micData = abs(digitalSample >> 16);

      vReal[i] = micData;                       // Store Mic Data in an array
      vImag[i] = 0;
      microseconds += sampling_period_us;
    }

    FFT.Windowing( FFT_WIN_TYP_HAMMING, FFT_FORWARD );      // Weigh data
    FFT.Compute( FFT_FORWARD );                             // Compute FFT
    FFT.ComplexToMagnitude();                               // Compute magnitudes

    //
    // vReal[3 .. 255] contain useful data, each a 20Hz interval (60Hz - 5120Hz).
    // There could be interesting data at bins 0 to 2, but there are too many artifacts.
    //
    FFT.MajorPeak(&FFT_MajorPeak, &FFT_Magnitude);          // let the effects know which freq was most dominant

    for (int i = 0; i < samples; i++) {                     // Values for bins 0 and 1 are WAY too large. Might as well start at 3.
      double t = 0.0;
      t = abs(vReal[i]);
      t = t / 16.0;                                         // Reduce magnitude. Want end result to be linear and ~4096 max.
      fftBin[i] = t;
    } // for()


    /* This FFT post processing is a DIY endeavour. What we really need is someone with sound engineering expertise to do a great job here AND most importantly, that the animations look GREAT as a result.
     *
     * Andrew's updated mapping of 256 bins down to the 16 result bins with Sample Freq = 10240, samples = 512 and some overlap.
     * Based on testing, the lowest/Start frequency is 60 Hz (with bin 3) and a highest/End frequency of 5120 Hz in bin 255.
     * Now, Take the 60Hz and multiply by 1.320367784 to get the next frequency and so on until the end. Then detetermine the bins.
     * End frequency = Start frequency * multiplier ^ 16
     * Multiplier = (End frequency/ Start frequency) ^ 1/16
     * Multiplier = 1.320367784
     */

                                          // Range
    fftCalc[0] = (fftAdd(3,4)) /2;        // 60 - 100
    fftCalc[1] = (fftAdd(4,5)) /2;        // 80 - 120
    fftCalc[2] = (fftAdd(5,7)) /3;        // 100 - 160
    fftCalc[3] = (fftAdd(7,9)) /3;        // 140 - 200
    fftCalc[4] = (fftAdd(9,12)) /4;       // 180 - 260
    fftCalc[5] = (fftAdd(12,16)) /5;      // 240 - 340
    fftCalc[6] = (fftAdd(16,21)) /6;      // 320 - 440
    fftCalc[7] = (fftAdd(21,28)) /8;      // 420 - 600
    fftCalc[8] = (fftAdd(29,37)) /10;     // 580 - 760
    fftCalc[9] = (fftAdd(37,48)) /12;     // 740 - 980
    fftCalc[10] = (fftAdd(48,64)) /17;    // 960 - 1300
    fftCalc[11] = (fftAdd(64,84)) /20;    // 1280 - 1700
    fftCalc[12] = (fftAdd(84,111)) /20;   // 1680 - 2240
    fftCalc[13] = (fftAdd(111,147)) /23;  // 2220 - 2960
    fftCalc[14] = (fftAdd(147,194)) /40;  // 2940 - 3900
    fftCalc[15] = (fftAdd(194, 255)) /50; // 3880 - 5120

   /* fftCalc[0] = (fftAdd(3,9)) /6;        // 60 - 100
    fftCalc[1] = (fftAdd(9,15)) /6;        // 80 - 120
    fftCalc[2] = (fftAdd(15,23)) /8;        // 100 - 160
    fftCalc[3] = (fftAdd(23,31)) /8;        // 140 - 200
    fftCalc[4] = (fftAdd(31,41)) /10;       // 180 - 260
    fftCalc[5] = (fftAdd(41,53)) /12;      // 240 - 340
    fftCalc[6] = (fftAdd(53,67)) /14;      // 320 - 440
    fftCalc[7] = (fftAdd(67,81)) /14;      // 420 - 600
    fftCalc[8] = (fftAdd(81,97)) /16;     // 580 - 760
    fftCalc[9] = (fftAdd(97,114)) /17;     // 740 - 980
    fftCalc[10] = (fftAdd(114,133)) /19;    // 960 - 1300
    fftCalc[11] = (fftAdd(133,154)) /21;    // 1280 - 1700
    fftCalc[12] = (fftAdd(154,178)) /24;   // 1680 - 2240
    fftCalc[13] = (fftAdd(178,206)) /28; // 2220 - 2960
    fftCalc[14] = (fftAdd(212,237)) /24;  // 2940 - 3900
    fftCalc[15] = (fftAdd(237, 255)) /24; // 3880 - 5120*/


    // Noise supression of fftCalc bins using squelch adjustment for different input types.
    for (int i=0; i < 16; i++) {
        fftCalc[i] = fftCalc[i]-(float)squelch*(float)linearNoise[i]/4.0 <= 0? 0 : fftCalc[i];
             //   Serial.print(i);Serial.print(' ');Serial.println(fftBin[i]);

    }

    // Adjustment for frequency curves.
    for (int i=0; i < 16; i++) {
      fftCalc[i] = fftCalc[i] * fftResultPink[i];
    }

    // Manual linear adjustment of gain using gain adjustment for different input types.
    for (int i=0; i < 16; i++) {
        fftCalc[i] = fftCalc[i] * gain / 40 + fftCalc[i]/16.0;
    }

    // Now, let's dump it all into fftResult. Need to do this, otherwise other routines might grab fftResult values prematurely.
    for (int i=0; i < 16; i++) {
        fftResult[i] = constrain((int)fftCalc[i],0,254);
        //Serial.println(fftResult[i]);
    }

  } // for(;;)
} // FFTcode()

void setupAudio() {
    
  // Attempt to configure INMP441 Microphone
  esp_err_t err;
  const i2s_config_t i2s_config = {
    .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),  // Receive, not transfer
    .sample_rate = SAMPLE_RATE*2,                       // 10240 * 2 (20480) Hz
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,       // could only get it to work with 32bits
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,        // LEFT when pin is tied to ground.
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,           // Interrupt level 1
    .dma_buf_count = 8,                                 // number of buffers
    .dma_buf_len = BLOCK_SIZE                           // samples per buffer
  };
  const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,      // BCLK aka SCK
    .ws_io_num = I2S_WS,        // LRCL aka WS
    .data_out_num = -1,         // not used (only for speakers)
    .data_in_num = I2S_SD       // DOUT aka SD
  };
  
  // Configuring the I2S driver and pins.
  // This function must be called before any I2S driver read/write operations.
  err = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  if (err != ESP_OK) {
    Serial.printf("Failed installing driver: %d\n", err);
    while (true);
  }
  err = i2s_set_pin(I2S_PORT, &pin_config);
  if (err != ESP_OK) {
    Serial.printf("Failed setting pin: %d\n", err);
    while (true);
  }
  Serial.println("I2S driver installed.");
  delay(100);


  // Test to see if we have a digital microphone installed or not.
  float mean = 0.0;
  int32_t samples[BLOCK_SIZE];
  size_t num_bytes_read = 0;

  esp_err_t result = i2s_read(I2S_PORT, &samples, BLOCK_SIZE, &num_bytes_read, portMAX_DELAY);
  
  /*int num_bytes_read = i2s_read_bytes(I2S_PORT,
                                      (char *)samples,
                                      BLOCK_SIZE,     // the doc says bytes, but its elements.
                                      portMAX_DELAY); // no timeout*/
  
  int samples_read = num_bytes_read / 8;
  if (samples_read > 0) {
    for (int i = 0; i < samples_read; ++i) {
      mean += samples[i];
    }
    mean = mean/BLOCK_SIZE/16384;
    if (mean != 0.0) {
      Serial.println("Digital microphone is present.");
    } else {
      Serial.println("Digital microphone is NOT present.");
    }
  }
  
  sampling_period_us = round(1000000*(1.0/SAMPLE_RATE));
  
  // Define the FFT Task and lock it to core 0
  xTaskCreatePinnedToCore(
        FFTcode,                          // Function to implement the task
        "FFT",                            // Name of the task
        10000,                            // Stack size in words
        NULL,                             // Task input parameter
        1,                                // Priority of the task
        &FFT_Task,                        // Task handle
        0);                               // Core where the task should run
} //setupAudio()
