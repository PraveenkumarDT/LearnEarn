/* Application for loudness measurement */
#include <stdio.h>
#include <sndfile.h>
#include <stdlib.h>
#include <string.h>

#include "ebur128.h"

/* sample_peak_window function to get sample peak for given window in msec */

void sample_peak_window(SNDFILE* file, SF_INFO file_info, ebur128_state* st, double* buffer, int window_msec,double* window_sample_peak) {
    sf_count_t nr_frames_read;
    int no_frames, sample_peak_return;
    double sample_peak;

    //setting window size in msec
    ebur128_set_max_window(st, window_msec);


    if (window_msec < 1000) {
        nr_frames_read = sf_readf_double(file, buffer, (sf_count_t)(st->samplerate * (window_msec/1000)));
        ebur128_add_frames_double(st, buffer, (size_t)nr_frames_read);
    }
    else
    {

        no_frames = (window_msec / 1000);
        while (no_frames > 0) {
            nr_frames_read = sf_readf_double(file, buffer, (sf_count_t)st->samplerate);
            ebur128_add_frames_double(st, buffer, (size_t)nr_frames_read);
            no_frames--;
        }

        long int remain_samples = (sf_count_t)(st->samplerate * (window_msec % 1000) / 1000);
        nr_frames_read = sf_readf_double(file, buffer, remain_samples);
        ebur128_add_frames_double(st, buffer, (size_t)nr_frames_read);
    }

    for (int ch = 0; ch < file_info.channels; ch++) {
        sample_peak_return = ebur128_sample_peak(st, ch, &sample_peak);
        window_sample_peak[ch] = sample_peak;
        //printf("sample_peak_level for given window:%d msec of channel-%d : %.4f\n", window_msec, ch, sample_peak);
    }
}



int main(int ac, const char* av[]) {
    SF_INFO file_info;
    SNDFILE* file;
    sf_count_t nr_frames_read;
    ebur128_state** sts = NULL;
    double* buffer, *window_samplepeak;
    double loudness, sample_peak, true_peak;
    int i, ch;


    if (ac < 3) {
        printf("usage: %s WINDOW_msec FILENAME ...\n", av[0]);
        exit(1);
    }

    sts = malloc((size_t)(ac - 2) *sizeof(ebur128_state*));
    if (!sts) {
        printf("malloc failed\n");
        return 1;
    }

    for (i = 0; i < ac - 2; ++i) {
        memset(&file_info, '\0', sizeof(file_info));
        file = sf_open(av[i+2], SFM_READ, &file_info);
        if (!file) {
            fprintf(stderr, "Could not open file with sf_open!\n");
            return 1;
        }

        if (DEB)
            printf("sample rate:%u\n", file_info.samplerate);

        //Initializing 
        sts[i] = ebur128_init((unsigned)file_info.channels, (unsigned)file_info.samplerate, (ebur128_MODE_I | ebur128_MODE_TRUE_PEAK));
        if (!sts[i]) {
            fprintf(stderr, "Could not create ebur128_state!\n");
            return 1;
        }

        // Buffer to read the data from file
        buffer = (double*)malloc(sts[i]->samplerate * sts[i]->channels * sizeof(double));
        if (!buffer) {
            printf("malloc failed- buffer\n");
            return 1;
        }

        window_samplepeak = (double*)malloc(sts[i]->channels * sizeof(double));
        if (!window_samplepeak) {
            printf("malloc failed -window_samplepeak\n");
            return 1;
        }
        printf("Input file: %s\n", av[i + 2]);

        // sample peak level for given window
        sample_peak_window(file, file_info, sts[i], buffer, atoi(av[1]),window_samplepeak);

        //setting window time back to 400 msec for loudness measurement 
        ebur128_set_max_window(sts[i], 400);

        while ((nr_frames_read = sf_readf_double(file, buffer, (sf_count_t)sts[i]->samplerate))) {
            ebur128_add_frames_double(sts[i], buffer, (size_t)nr_frames_read);
        }

        ebur128_loudness_global(sts[i], &loudness);
        printf("Global loudness : %.4f LKFS\n", loudness);
		

        for (ch = 0; ch < file_info.channels; ch++)
        {
            ebur128_sample_peak(sts[i], ch, &sample_peak);
            if (sample_peak != window_samplepeak[ch]) {
                printf("channel:%d -sample peak level in given %s msec window : 0\n", ch, av[1]);
            }
                
            else {
                printf("channel:%d -sample peak level in given %s msec window : 1\n", ch, av[1]);
            }
                
            //printf("sample_peak_level for given window:%s msec of channel-%d : %.4f\n", av[1], ch, window_samplepeak[ch]);
            //printf("sample_peak value of channel-%d : %.4f\n", ch, sample_peak);

            ebur128_true_peak(sts[i], ch, &true_peak);
            printf("True_peak value of channel-%d : %.4f\n", ch, true_peak);
        }

        free(buffer);
        buffer = NULL;

        if (sf_close(file)) {
            printf("Could not close input file!\n");
        }
    }

    /* clean up */
    for (i = 0; i < ac - 1; ++i) {
        ebur128_destroy(&sts[i]);
    }
    free(sts);

    return 0;
}
