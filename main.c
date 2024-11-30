#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <argp.h>

#define __USE_MISC 1
#include <math.h>

#define SAMPLE_RATE     44100
#define BIT_DEPTH       16

struct SineOscillator {
    float freq;
    float amp;
    float angle;
    float offset;
};

struct SineOscillator * sinosc_create(float _freq, float _amp) {
    struct SineOscillator * so = malloc(sizeof(struct SineOscillator));
    so->freq = _freq;
    so->amp = _amp;
    so->angle = 0.0f;
    so->offset = 2.0f * M_PI * so->freq / SAMPLE_RATE;

    return so;
}

float sinosc_process(struct SineOscillator *so) {
    float sample = so->amp * sinf(so->angle);
    so->angle += so->offset;

    return sample;
}

int count_char_ptr(char *p) {
    uint32_t i = 0;

    while (p[i] != '\0') { i++; }

    return i;
}

/*
 * To handle writing integer to file 
 * by converting a 4 bytes long into a desired array of size byte(s) long
 *
 * @param
 *  bytes: The bytes to write out
 *
 *  size : Size for each byte
 *  
 *  file : FILE struct
 */
void write_int_bytes(int bytes, int size, FILE *file) {
    char *byte = malloc(size);
    for (int i = 0; i < size; i++) {
        byte[i] = (bytes >> (8 * i)) & 0xFF;
    }

    fwrite(byte, sizeof(*byte), size, file);

    free(byte);
    byte = NULL;
}

int main(int argc, char **argv) {
    uint32_t duration = 2;

    if (argc == 2) {
        duration = atoi(*(argv + 1));
    } else if (argc > 2) {
        printf(
            "Example usage:\n"
            "\t./swo"
            "\t./swo [duration]\n"
            "\nHelp:\n"
            "\t(Optional) duration in seconds; The default duration is 2 seconds.\n"
        );

        return 1;
    }

    struct SineOscillator *so = sinosc_create(440.0f, 0.5f);

    const float max_amp = powf(2.0f, BIT_DEPTH - 1.0f) - 1.0f;

    FILE *file = fopen("sine_wave.wav", "wb");
    if (!file) {
        printf("Failed to create a file data\n");
        exit(EXIT_FAILURE);
    }

    // Write header chunk
    fwrite("RIFF", 1, 4, file);
    fwrite("----", 1, 4, file);
    fwrite("WAVE", 1, 4, file);

    // Write format chunk
    fwrite("fmt ", 1, 4, file);
    write_int_bytes(16, 4, file);
    write_int_bytes(1, 2, file);
    write_int_bytes(1, 2, file);
    write_int_bytes(SAMPLE_RATE, 4, file);
    write_int_bytes(SAMPLE_RATE * BIT_DEPTH / 8, 4, file);
    write_int_bytes(BIT_DEPTH / 8, 2, file);
    write_int_bytes(BIT_DEPTH, 2, file);

    // Data chunk
    fwrite("data", 1, 4, file);
    fwrite("----", 1, 4, file);

    long pre_positon = ftell(file);

    for (int i = 0; i < SAMPLE_RATE * duration; i++) {
        float sample = sinosc_process(so);
        int int_sample = (int)(sample * max_amp);

        write_int_bytes(int_sample, 2, file);
    }

    long post_position = ftell(file);
    int byte_pos = fseek(file, pre_positon - 4, SEEK_SET);

    write_int_bytes(post_position - pre_positon, 4, file);
    write_int_bytes(post_position - 8, 4, file);

    fseek(file, 4, SEEK_SET);
    
    fclose(file);
    free(so);
    so = NULL;
    
    return 0;
}
