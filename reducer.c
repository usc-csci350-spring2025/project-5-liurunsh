#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>

#define MAX_WORD_LEN 256
#define BUFFER_SIZE 4096

struct WordCount {
    char word[MAX_WORD_LEN];
    int count;
    struct WordCount *next;
};

struct WordCount *word_counts = NULL;

void add_word(const char *word, int count) {
    // Skip empty words
    if (!word || word[0] == '\0') {
        return;
    }
    
    // Create a copy of the word to normalize
    char normalized[MAX_WORD_LEN];
    strncpy(normalized, word, MAX_WORD_LEN - 1);
    normalized[MAX_WORD_LEN - 1] = '\0';
    
    // Remove any trailing whitespace
    int len = strlen(normalized);
    while (len > 0 && isspace(normalized[len - 1])) {
        normalized[--len] = '\0';
    }
    
    // Skip if empty after normalization
    if (len == 0) {
        return;
    }
    
    struct WordCount *curr = word_counts;
    while (curr) {
        if (strcmp(curr->word, normalized) == 0) {
            curr->count += count;
            return;
        }
        curr = curr->next;
    }
    
    struct WordCount *new_word = malloc(sizeof(struct WordCount));
    strncpy(new_word->word, normalized, MAX_WORD_LEN - 1);
    new_word->word[MAX_WORD_LEN - 1] = '\0';
    new_word->count = count;
    new_word->next = word_counts;
    word_counts = new_word;
}

int compare(struct WordCount *a, struct WordCount *b) {
    return strcmp(a->word, b->word);
}

void merge(struct WordCount **arr, int left, int mid, int right) {
    int n1 = mid - left + 1;
    int n2 = right - mid;
    
    // Create temporary arrays
    struct WordCount **L = malloc(n1 * sizeof(struct WordCount *));
    struct WordCount **R = malloc(n2 * sizeof(struct WordCount *));
    
    // Copy data to temporary arrays
    for (int i = 0; i < n1; i++)
        L[i] = arr[left + i];
    for (int j = 0; j < n2; j++)
        R[j] = arr[mid + 1 + j];
    
    // Merge the temporary arrays back
    int i = 0, j = 0, k = left;
    while (i < n1 && j < n2) {
        if (compare(L[i], R[j]) >= 0) {
            arr[k] = L[i];
            i++;
        } else {
            arr[k] = R[j];
            j++;
        }
        k++;
    }
    
    // Copy remaining elements
    while (i < n1) {
        arr[k] = L[i];
        i++;
        k++;
    }
    while (j < n2) {
        arr[k] = R[j];
        j++;
        k++;
    }
    
    free(L);
    free(R);
}

void mergeSort(struct WordCount **arr, int left, int right) {
    if (left < right) {
        int mid = left + (right - left) / 2;
        mergeSort(arr, left, mid);
        mergeSort(arr, mid + 1, right);
        merge(arr, left, mid, right);
    }
}

void output_results() {
    // Count the number of words
    int count = 0;
    struct WordCount *curr = word_counts;
    while (curr) {
        count++;
        curr = curr->next;
    }
    
    // Create an array of pointers to WordCount structs
    struct WordCount **arr = malloc(count * sizeof(struct WordCount *));
    curr = word_counts;
    for (int i = 0; i < count; i++) {
        arr[i] = curr;
        curr = curr->next;
    }
    
    // Sort the array
    mergeSort(arr, 0, count - 1);
    
    // Output the sorted results
    for (int i = 0; i < count; i++) {
        printf("%s %d\n", arr[i]->word, arr[i]->count);
        fflush(stdout);
    }
    
    free(arr);
}

int main() {
    // Set stdin to non-blocking
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    
    // Set stdout to unbuffered
    setvbuf(stdout, NULL, _IONBF, 0);
    
    char buffer[BUFFER_SIZE];
    char partial_line[BUFFER_SIZE] = "";
    int partial_len = 0;
    int eof_reached = 0;
    int no_data_count = 0;
    
    while (!eof_reached || partial_len > 0) {
        ssize_t n = read(STDIN_FILENO, buffer, sizeof(buffer));
        
        if (n > 0) {
            no_data_count = 0;
            // Process the input
            char word[MAX_WORD_LEN];
            int count;
            
            // Append new data to partial line
            if (partial_len + n >= sizeof(partial_line)) {
                // Buffer overflow, process partial line first
                char *newline = strchr(partial_line, '\n');
                if (newline) {
                    *newline = '\0';
                    if (sscanf(partial_line, "%255s %d", word, &count) == 2) {
                        add_word(word, count);
                    }
                    memmove(partial_line, newline + 1, partial_len - (newline - partial_line + 1));
                    partial_len -= (newline - partial_line + 1);
                } else {
                    // No newline in partial line, must be corrupted
                    partial_len = 0;
                }
            }
            
            memcpy(partial_line + partial_len, buffer, n);
            partial_len += n;
            partial_line[partial_len] = '\0';
            
            // Process complete lines
            char *start = partial_line;
            char *newline;
            while ((newline = strchr(start, '\n'))) {
                *newline = '\0';
                char *space = strrchr(start, ' ');
                if (space) {
                    *space = '\0';
                    count = atoi(space + 1);
                    strncpy(word, start, MAX_WORD_LEN - 1);
                    word[MAX_WORD_LEN - 1] = '\0';
                    add_word(word, count);
                }
                start = newline + 1;
            }
            
            // Move remaining partial line to beginning
            partial_len = partial_line + partial_len - start;
            if (partial_len > 0) {
                memmove(partial_line, start, partial_len);
            }
            
        } else if (n == 0) {
            eof_reached = 1;
        } else {
            if (errno != EAGAIN && errno != EINTR) {
                perror("read");
                exit(1);
            }
            no_data_count++;
            if (no_data_count > 1000) {
                usleep(1000); // Sleep 1ms if no data for a while
                no_data_count = 0;
            }
        }
    }
    
    output_results();
    return 0;
}
