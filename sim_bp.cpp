#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include "sim_bp.h"

using namespace std;

unsigned long int predictions = 0;
unsigned long int mispredictions = 0;
uint32_t gshare_reg = 0;

vector<uint8_t> bimodal_counter;
vector<uint8_t> gshare_counter;
vector<uint8_t> chooser_counter;

bool Bimodal(unsigned long int addr, char t_nt, unsigned long int counter_bits, bool bimodal_flag = true) {
    unsigned long int address;
    bool prediction_correct;
    unsigned long int mask = (1UL << counter_bits) - 1;
    address = (addr >> 2) & mask;

    if (t_nt == 't') {
        if (bimodal_counter[address] >= 2) {
            if (bimodal_flag) predictions++;
            prediction_correct = true;
        } else {
            if (bimodal_flag) mispredictions++;
            prediction_correct = false;
        }
        if (bimodal_counter[address] >= 3) bimodal_counter[address] = 3;
        else if (bimodal_flag) bimodal_counter[address]++;
    } else if (t_nt == 'n') {
        if (bimodal_counter[address] <= 1) {
            if (bimodal_flag) predictions++;
            prediction_correct = true;
        } else {
            if (bimodal_flag) mispredictions++;
            prediction_correct = false;
        }
        if (bimodal_counter[address] <= 0) bimodal_counter[address] = 0;
        else if (bimodal_flag) bimodal_counter[address]--;
    }

    return prediction_correct;
}

bool Gshare(unsigned long int addr, char t_nt, unsigned long int counter_bits, unsigned long int gshare_width, bool gshare_flag = true) {
    unsigned long int address;
    bool prediction_correct;
    unsigned long int mask1 = (1UL << counter_bits) - 1;
    address = (addr >> 2) & mask1;
    unsigned long int mask2 = (1UL << gshare_width) - 1;
    unsigned long int gshare_bits = gshare_reg & mask2;
    unsigned long int upper_bits = (address >> (counter_bits - gshare_width)) & mask2;
    unsigned long int upperbits_xor = gshare_bits ^ upper_bits;
    unsigned long int index = (upperbits_xor << (counter_bits - gshare_width)) | (address & ((1UL << (counter_bits - gshare_width)) - 1));

    if (t_nt == 't') {
        if (gshare_counter[index] >= 2) {
            if (gshare_flag) predictions++;
            prediction_correct = true;
        } else {
            if (gshare_flag) mispredictions++;
            prediction_correct = false;
        }
        if (gshare_counter[index] >= 3) gshare_counter[index] = 3;
        else if (gshare_flag) gshare_counter[index]++;
    } else if (t_nt == 'n') {
        if (gshare_counter[index] <= 1) {
            if (gshare_flag) predictions++;
            prediction_correct = true;
        } else {
            if (gshare_flag) mispredictions++;
            prediction_correct = false;
        }
        if (gshare_counter[index] <= 0) gshare_counter[index] = 0;
        else if (gshare_flag) gshare_counter[index]--;
    }

    gshare_bits = (gshare_bits >> 1) & mask2;
    gshare_bits |= (t_nt == 't' ? 1UL : 0UL) << (gshare_width - 1);
    gshare_reg = (gshare_reg & ~mask2) | gshare_bits;

    return prediction_correct;
}

void Hybrid(unsigned long int addr, char t_nt, unsigned long int chooser_bits, unsigned long int gshare_bits, unsigned long int gshare_bhr_bits, unsigned long int bimodal_bits, unsigned long int counter) {
    unsigned long int address;
    unsigned long int mask1 = (1UL << chooser_bits) - 1;
    address = (addr >> 2) & mask1;
    bool bimodal, gshare, gshare_flag, bimodal_flag;

    if (chooser_counter[address] >= 2) {
        gshare = true;
        bimodal = false;
    } else {
        bimodal = true;
        gshare = false;
    }

    gshare_flag = Gshare(addr, t_nt, gshare_bits, gshare_bhr_bits, gshare);
    bimodal_flag = Bimodal(addr, t_nt, bimodal_bits, bimodal);

    if (gshare_flag && !bimodal_flag) {
        if (chooser_counter[address] >= 3) chooser_counter[address] = 3;
        else chooser_counter[address]++;
    } else if (!gshare_flag && bimodal_flag) {
        if (chooser_counter[address] <= 0) chooser_counter[address] = 0;
        else chooser_counter[address]--;
    }
}

void print_predict(unsigned long int count) {
    printf("OUTPUT\n");
    printf(" number of predictions:    %lu\n", count);
    printf(" number of mispredictions: %lu\n", mispredictions);
    double result = (double(mispredictions) / double(count)) * 100;
    printf(" misprediction rate:       %.2f%%\n", result);
    //printf("%.2f\n", result);
}

void print_chooser_contents() {
    printf("FINAL CHOOSER CONTENTS\n");
    for (unsigned long int i = 0; i < chooser_counter.size(); i++) {
        printf(" %lu\t%d\n", i, chooser_counter[i]);
    }
}

void print_gshare_contents() {
    printf("FINAL GSHARE CONTENTS\n");
    for (unsigned long int i = 0; i < gshare_counter.size(); i++) {
        printf(" %lu\t%d\n", i, gshare_counter[i]);
    }
}

void print_bimodal_contents() {
    printf("FINAL BIMODAL CONTENTS\n");
    for (unsigned long int i = 0; i < bimodal_counter.size(); i++) {
        printf(" %lu\t%d\n", i, bimodal_counter[i]);
    }
}

int main(int argc, char* argv[]) {
    FILE *FP;
    char *trace_file;
    bp_params params;
    char outcome;
    unsigned long int addr;
    unsigned long int counter = 0;

    if (!(argc == 4 || argc == 5 || argc == 7)) {
        printf("Error: Wrong number of inputs:%d\n", argc-1);
        exit(EXIT_FAILURE);
    }

    params.bp_name = argv[1];

    if (strcmp(params.bp_name, "bimodal") == 0) {
        if (argc != 4) {
            printf("Error: %s wrong number of inputs:%d\n", params.bp_name, argc-1);
            exit(EXIT_FAILURE);
        }
        params.M2 = strtoul(argv[2], NULL, 10);
        trace_file = argv[3];
        printf("COMMAND\n%s %s %lu %s\n", argv[0], params.bp_name, params.M2, trace_file);
        bimodal_counter.resize(1UL << params.M2, 2);
    } else if (strcmp(params.bp_name, "gshare") == 0) {
        if (argc != 5) {
            printf("Error: %s wrong number of inputs:%d\n", params.bp_name, argc-1);
            exit(EXIT_FAILURE);
        }
        params.M1 = strtoul(argv[2], NULL, 10);
        params.N = strtoul(argv[3], NULL, 10);
        trace_file = argv[4];
        printf("COMMAND\n%s %s %lu %lu %s\n", argv[0], params.bp_name, params.M1, params.N, trace_file);
        gshare_counter.resize(1UL << params.M1, 2);
    } else if (strcmp(params.bp_name, "hybrid") == 0) {
        if (argc != 7) {
            printf("Error: %s wrong number of inputs:%d\n", params.bp_name, argc-1);
            exit(EXIT_FAILURE);
        }
        params.K = strtoul(argv[2], NULL, 10);
        params.M1 = strtoul(argv[3], NULL, 10);
        params.N = strtoul(argv[4], NULL, 10);
        params.M2 = strtoul(argv[5], NULL, 10);
        trace_file = argv[6];
        printf("COMMAND\n%s %s %lu %lu %lu %lu %s\n", argv[0], params.bp_name, params.K, params.M1, params.N, params.M2, trace_file);
        chooser_counter.resize(1UL << params.K, 1);
        gshare_counter.resize(1UL << params.M1, 2);
        bimodal_counter.resize(1UL << params.M2, 2);
    } else {
        printf("Error: Wrong branch predictor name:%s\n", params.bp_name);
        exit(EXIT_FAILURE);
    }

    FP = fopen(trace_file, "r");
    if (FP == NULL) {
        printf("Error: Unable to open file %s\n", trace_file);
        exit(EXIT_FAILURE);
    }

    char str[2];
    while (fscanf(FP, "%lx %s", &addr, str) != EOF) {
        outcome = str[0];
        if (strcmp(params.bp_name, "bimodal") == 0) {
            Bimodal(addr, outcome, params.M2);
        } else if (strcmp(params.bp_name, "gshare") == 0) {
            Gshare(addr, outcome, params.M1, params.N);
        } else if (strcmp(params.bp_name, "hybrid") == 0) {
            Hybrid(addr, outcome, params.K, params.M1, params.N, params.M2, counter);
        }
        counter++;
    }

    print_predict(counter);

    if (strcmp(params.bp_name, "hybrid") == 0) {
        print_chooser_contents();
        print_gshare_contents();
        print_bimodal_contents();
    } else if (strcmp(params.bp_name, "gshare") == 0) {
        print_gshare_contents();
    } else if (strcmp(params.bp_name, "bimodal") == 0) {
        print_bimodal_contents();
    }

    fclose(FP);
    return 0;
}