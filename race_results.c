#include "race_results.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// This is the (somewhat famous) djb2 hash
unsigned hash(const char *str) {
    unsigned hash_val = 5381;
    int i = 0;
    while (str[i] != '\0') {
        hash_val = ((hash_val << 5) + hash_val) + str[i];
        i++;
    }
    return hash_val % NUM_BUCKETS;
}

results_log_t *create_results_log(const char *log_name) {
    results_log_t *log = (results_log_t *) malloc(sizeof(results_log_t));
    if (log == NULL) {
        return NULL;
    }

    if (log_name != NULL) {
        strncpy(log->name, log_name, NAME_LEN - 1);
        log->name[NAME_LEN - 1] = '\0';
    } else {
        log->name[0] = '\0';
    }

    log->size = 0;
    for (int i = 0; i < NUM_BUCKETS; i++) {
        log->buckets[i] = NULL;
    }

    return log;
}

const char *get_results_log_name(const results_log_t *log) {
    if (log == NULL) {
        return NULL;
    }

    return log->name;
}

int add_participant(results_log_t *log, const char *name, unsigned age, unsigned time_seconds) {
    unsigned index = hash(name);
    unsigned index_at_start = index;

    if (log == NULL) {
        printf("Error: You must create or load a results log first\n");
        return -1;
    }

    participant_t *new_participant = malloc(sizeof(participant_t));
    if (new_participant == NULL) {
        return -1;
    } else {
        strncpy(new_participant->name, name, NAME_LEN - 1);
        new_participant->name[NAME_LEN - 1] = '\0';
        new_participant->age = age;
        new_participant->time_seconds = time_seconds;
    }

    while (log->buckets[index] != NULL) {
        index = (index + 1) % NUM_BUCKETS;
        if (index == index_at_start) {
            free(new_participant);
            return -1;
        }
    }

    log->buckets[index] = new_participant;
    log->size++;

    return 0;
}

const participant_t *find_participant(const results_log_t *log, const char *name) {
    unsigned index = hash(name);

    if (log == NULL) {
        printf("Error: You must create or load a results log first\n");
        return NULL;
    }

    participant_t *participant = log->buckets[index];
    while (participant != NULL) {
        if (strcmp(participant->name, name) == 0) {
            printf("%s\n", participant->name);
            printf("Age: %u\n", participant->age);
            printf("Time: ");
            print_formatted_time(participant->time_seconds);
            printf("\n");
            return participant;
        }
        index = (index + 1) % NUM_BUCKETS;
    }

    printf("No participant found with name '%s'\n", name);

    return NULL;
}

void print_formatted_time(unsigned time_seconds) {
    unsigned hours = time_seconds / (60 * 60);
    time_seconds %= (60 * 60);
    unsigned minutes = time_seconds / 60;
    time_seconds %= 60;
    printf("%u:%02u:%02u", hours, minutes, time_seconds);
}

void print_results_log(const results_log_t *log) {
    if (log == NULL) {
        printf("Error: You must create or load a results log first\n");
        return;
    }

    printf("%s Results\n", log->name);

    for (int i = 0; i < NUM_BUCKETS; i++) {
        participant_t *participant = log->buckets[i];
        if (participant != NULL) {
            printf("Name: %s\n", participant->name);
            printf("Age: %u\n", participant->age);
            printf("Time: ");
            print_formatted_time(participant->time_seconds);
            printf("\n");
        }
    }
}

void free_results_log(results_log_t *log) {
    if (log == NULL) {
        printf("Error: No results log to clear\n");
        return;
    }

    for (int i = 0; i < NUM_BUCKETS; i++) {
        participant_t *participant = log->buckets[i];
        if (participant != NULL) {
            free(participant);
            log->buckets[i] = NULL;
        }
    }
    free(log);
    log = NULL;
}

int write_results_log_to_text(const results_log_t *log) {
    char file_name[NAME_LEN];
    strcpy(file_name, log->name);
    strcat(file_name, ".txt");

    FILE *f = fopen(file_name, "w");
    if (f == NULL) {
        return -1;
    }

    fprintf(f, "%u\n", log->size);
    for (int i = 0; i < NUM_BUCKETS; i++) {
        participant_t *participant = log->buckets[i];
        if (participant != NULL) {
            fprintf(f, "%s %u %u\n", participant->name, participant->age,
                    participant->time_seconds);
        }
    }

    fclose(f);
    return 0;
}

results_log_t *read_results_log_from_text(const char *file_name) {
    char results_log_name[NAME_LEN] = {0};
    char participant_name[NAME_LEN];
    unsigned age;
    unsigned total_seconds;
    unsigned size_of_log;
    int i;

    for (i = 0; i < NAME_LEN - 1 && file_name[i] != '\0'; i++) {
        if (file_name[i] != '.') {
            results_log_name[i] = file_name[i];
        }
    }
    results_log_name[i] = '\0';

    results_log_t *log = create_results_log(results_log_name);
    if (log == NULL) {
        printf("Error: You must clear current results log first\n");
        return NULL;
    }

    FILE *f = fopen(file_name, "r");
    if (f == NULL) {
        printf("Failed to read results log from text file\n");
        free_results_log(log);
        return NULL;
    }

    fscanf(f, "%u", &size_of_log);

    for (int k = 0; k < size_of_log; k++) {
        fscanf(f, "%s %u %u", participant_name, &age, &total_seconds);
        add_participant(log, participant_name, age, total_seconds);
    }

    fclose(f);
    printf("Results log loaded from text file\n");
    return log;
}

int write_results_log_to_binary(const results_log_t *log) {
    char file_name[NAME_LEN];
    strcpy(file_name, log->name);
    strcat(file_name, ".bin");

    FILE *f = fopen(file_name, "wb");
    if (f == NULL) {
        return -1;
    }

    fwrite(&log->size, sizeof(unsigned), 1, f);
    for (int i = 0; i < NUM_BUCKETS; i++) {
        participant_t *participant = log->buckets[i];
        if (participant != NULL) {
            unsigned name_len = strlen(participant->name);
            fwrite(&name_len, sizeof(unsigned), 1, f);
            fwrite(participant->name, name_len, 1, f);
            fwrite(&participant->age, sizeof(unsigned), 1, f);
            fwrite(&participant->time_seconds, sizeof(unsigned), 1, f);
        }
    }

    fclose(f);
    return 0;
}

results_log_t *read_results_log_from_binary(const char *file_name) {
    char results_log_name[NAME_LEN] = {0};
    char participant_name[NAME_LEN];
    unsigned age;
    unsigned total_seconds;
    unsigned size_of_log;
    int i;

    for (i = 0; i < NAME_LEN - 1 && file_name[i] != '\0'; i++) {
        if (file_name[i] != '.') {
            results_log_name[i] = file_name[i];
        }
    }
    results_log_name[i] = '\0';

    results_log_t *log = create_results_log(results_log_name);
    if (log == NULL) {
        printf("Error: You must clear current results log first\n");
        return NULL;
    }

    FILE *f = fopen(file_name, "rb");
    if (f == NULL) {
        printf("Failed to read results log from binary file\n");
        free_results_log(log);
        return NULL;
    }

    fread(&size_of_log, sizeof(unsigned), 1, f);

    for (int k = 0; k < size_of_log; k++) {
        unsigned name_length;
        fread(&name_length, sizeof(unsigned), 1, f);
        fread(participant_name, name_length, 1, f);
        participant_name[name_length] = '\0';
        fread(&age, sizeof(unsigned), 1, f);
        fread(&total_seconds, sizeof(unsigned), 1, f);
        add_participant(log, participant_name, age, total_seconds);
    }

    fclose(f);
    printf("Results log loaded from binary file\n");
    return log;
}
