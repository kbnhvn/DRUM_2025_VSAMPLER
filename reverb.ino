
#define REVERB_DELAY_SAMPLES 20000//4410
#define MIN_DELAY_SAMPLES 10     // 441       
#define MAX_DELAY_SAMPLES REVERB_DELAY_SAMPLES  // Máximo delay

// Feedback (0.0 a 1.0)
#define REVERB_FEEDBACK 0.5f

// Buffers circulares para cada canal 
int16_t reverbBufferL[REVERB_DELAY_SAMPLES] = {0};
int16_t reverbBufferR[REVERB_DELAY_SAMPLES] = {0};

// Índices para recorrer cada buffer de retardo
uint16_t reverbIndexL = 0;
uint16_t reverbIndexR = 0;

uint16_t delayOffsetSamples = 0;

volatile float reverbMix = 1;//0.5f; 

int16_t applyReverbChannel(int16_t input, int16_t *buffer, uint16_t &index, uint16_t delayOffset) {
    // Calcula el índice de lectura en el buffer para obtener la muestra retrasada.
    // Se toma la posición actual 'index' y se retrocede 'delayOffset' muestras (usando aritmética modular).
    uint16_t readIndex = (index + REVERB_DELAY_SAMPLES - delayOffset) % REVERB_DELAY_SAMPLES;
    int16_t delayedSample = buffer[readIndex];

    // Mezcla la señal original y la retardada según reverbMix:
    // Con reverbMix = 0 se escucha solo la original,
    // con reverbMix = 1 se escucha solo la señal retrasada.
    //int32_t output = (int32_t)input * (1.0f - reverbMix) + (int32_t)delayedSample * reverbMix;
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        int32_t output = (int32_t)input  + (int32_t)delayedSample * reverbMix;
    // Calcula el feedback y actualiza el buffer:
    int32_t newBufferValue = input + (int32_t)delayedSample * REVERB_FEEDBACK;
    if(newBufferValue > INT16_MAX) newBufferValue = INT16_MAX;
    if(newBufferValue < INT16_MIN) newBufferValue = INT16_MIN;
    buffer[index] = (int16_t)newBufferValue;

    // Avanza el índice del buffer de forma circular.
    index = (index + 1) % REVERB_DELAY_SAMPLES;

    return (int16_t)output;
}

