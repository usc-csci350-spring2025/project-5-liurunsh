// Compile: gcc -O2 mapper.c -o mapper
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <stdbool.h>

#define MAX_WORD_LEN 256
#define BUFFER_SIZE 4096

// Helper function to check if a word ends with a pattern
int ends_with(const char* word, const char* pattern) {
    int word_len = strlen(word);
    int pattern_len = strlen(pattern);
    if (word_len < pattern_len) return 0;
    return strcmp(word + word_len - pattern_len, pattern) == 0;
}

void normalize_word(const char* word, char* normalized) {
    if (!word || !*word) {
        normalized[0] = '\0';
        return;
    }

    int len = strlen(word);
    int i = 0, j = 0;
    int has_letter = 0;
    int prev_was_alpha = 0;

    while (i < len && j < MAX_WORD_LEN - 1) {
        char c = tolower(word[i]);

        if (isalpha(c)) {
            normalized[j++] = c;
            has_letter = 1;
            prev_was_alpha = 1;
        } else if (c == '\'' && prev_was_alpha && i < len - 1) {
            char next = tolower(word[i + 1]);
            if (next == 's' && (i + 2 == len || !isalpha(word[i + 2]))) {
                // Handle possessives (word's)
                normalized[j++] = 's';
                i++;
            } else if (next == 't' && (i + 2 == len || !isalpha(word[i + 2]))) {
                // Handle contractions (word't)
                normalized[j++] = 't';
                i++;
            }
        } else if (c == '-' && prev_was_alpha && i < len - 1 && isalpha(word[i + 1])) {
            // Skip hyphens in compound words
            prev_was_alpha = 0;
        } else {
            prev_was_alpha = 0;
        }
        i++;
    }
    normalized[j] = '\0';

    // Only keep the word if it has at least one letter
    if (!has_letter) {
        normalized[0] = '\0';
    }
}

int is_hyphenated(const char* word) {
    char* hyphen = strchr(word, '-');
    return hyphen && hyphen > word && *(hyphen + 1) != '\0' &&
           isalpha(*(hyphen - 1)) && isalpha(*(hyphen + 1));
}

int is_possessive(const char* word) {
    int len = strlen(word);
    return len > 2 && word[len-2] == '\'' && word[len-1] == 's';
}

int is_contraction(const char* word) {
    char* apos = strchr(word, '\'');
    if (!apos || apos == word || *(apos + 1) == '\0') return 0;
    
    char next = tolower(*(apos + 1));
    return next == 't' || next == 's' ||
           (*(apos + 2) != '\0' && 
            ((next == 'v' && tolower(*(apos + 2)) == 'e') ||
             (next == 'l' && tolower(*(apos + 2)) == 'l') ||
             (next == 'r' && tolower(*(apos + 2)) == 'e')));
}

void process_hyphenated(const char* word, char* result) {
    int i = 0, j = 0;
    while (word[i]) {
        if (word[i] != '-') {
            result[j++] = tolower(word[i]);
        }
        i++;
    }
    result[j] = '\0';
}

void process_possessive(const char* word, char* result) {
    int i = 0, j = 0;
    while (word[i] && word[i] != '\'') {
        result[j++] = tolower(word[i++]);
    }
    result[j++] = 's';
    result[j] = '\0';
}

void process_contraction(const char* word, char* result) {
    char* apos = strchr(word, '\'');
    int i = 0, j = 0;
    
    // Copy up to apostrophe
    while (word[i] && word[i] != '\'') {
        result[j++] = tolower(word[i++]);
    }
    
    // Skip apostrophe
    i++;
    
    // Copy rest of word
    while (word[i]) {
        result[j++] = tolower(word[i++]);
    }
    result[j] = '\0';
}

void process_regular(const char* word, char* result) {
    int i = 0;
    while (word[i]) {
        if (isalpha(word[i])) {
            result[i] = tolower(word[i]);
        }
        i++;
    }
    result[i] = '\0';
}

// Returns true if the character is a valid word character
bool is_word_char(char c) {
    return isalpha(c) || c == '\'' || c == '-';
}

// Returns true if the word contains a contraction
bool has_contraction(const char *word) {
    return strstr(word, "'st") != NULL || 
           strstr(word, "'d") != NULL || 
           strstr(word, "'ll") != NULL || 
           strstr(word, "'ve") != NULL || 
           strstr(word, "'re") != NULL || 
           strstr(word, "'m") != NULL;
}

void process_word(char *word) {
    if (!word || strlen(word) == 0) return;

    char processed[256] = {0};
    int proc_len = 0;
    
    // 只保留字母并转换为小写
    for (int i = 0; word[i] != '\0'; i++) {
        if (isalpha(word[i])) {
            processed[proc_len++] = tolower(word[i]);
        }
    }
    
    processed[proc_len] = '\0';
    
    // 只有当结果不为空时才输出
    if (proc_len > 0) {
        // printf("%s 1\n", processed);
    }
}

// 预处理输入行，处理特殊情况
void preprocess_line(char *line) {
    int len = strlen(line);
    
    // 将所有's替换为s
    for (int i = 0; i < len - 1; i++) {
        if (line[i] == '\'' && line[i+1] == 's' && (i+2 == len || !isalpha(line[i+2]))) {
            line[i] = 's';
            // 移除多余的字符
            memmove(&line[i+1], &line[i+2], len - i - 1);
            len--;
        }
    }
    
    // 将所有't替换为t
    for (int i = 0; i < len - 1; i++) {
        if (line[i] == '\'' && line[i+1] == 't' && (i+2 == len || !isalpha(line[i+2]))) {
            line[i] = 't';
            // 移除多余的字符
            memmove(&line[i+1], &line[i+2], len - i - 1);
            len--;
        }
    }
    
    // 将所有-替换为空字符
    for (int i = 0; i < len; i++) {
        if (line[i] == '-' && i > 0 && i < len - 1 && isalpha(line[i-1]) && isalpha(line[i+1])) {
            // 移除连字符
            memmove(&line[i], &line[i+1], len - i);
            len--;
            i--; // 回退一位，因为我们移除了一个字符
        }
    }
}


void clean_smart_quotes(char* str) {
    int i = 0, j = 0;
    while (str[i]) {
        // Skip smart quotes
        if ((unsigned char)str[i] == 0xE2 &&
            (unsigned char)str[i + 1] == 0x80 &&
            ((unsigned char)str[i + 2] == 0x98 ||  // ‘
             (unsigned char)str[i + 2] == 0x99) ||
             (unsigned char)str[i + 2] == 0x94)   // ’
        {

            i += 3;
            continue;
        } else {
            str[j++] = str[i++];
        }
    }
    str[j] = '\0';
}

// 修改字符串，处理缩写和连字符
void normalize_string(char* str) {
    clean_smart_quotes(str);
    int len = strlen(str);
    
    // 转换为小写
    for (int i = 0; i < len; i++) {
        str[i] = tolower((unsigned char)str[i]);
    }
    // 替换 Unicode 引号为普通撇号
for (int i = 0; str[i]; i++) {
    if ((unsigned char)str[i] == 0xE2 && 
        (unsigned char)str[i + 1] == 0x80 &&
        ((unsigned char)str[i + 2] == 0x99 ||  // ’
         (unsigned char)str[i + 2] == 0x9C ||  // “
         (unsigned char)str[i + 2] == 0x9D))   // ”
    {
        str[i] = '\'';
        memmove(&str[i + 1], &str[i + 3], strlen(&str[i + 3]) + 1);
    }
}

    // 处理所有格（将"beauty's"变为"beautys"）
    char *pos = strstr(str, "'s");
    while (pos != NULL) {
        // 替换's为s
        *pos = 's';
        // 删除后面的字符
        memmove(pos + 1, pos + 2, strlen(pos + 2) + 1);
        len--;
        
        // 查找下一个's
        pos = strstr(str, "'s");
    }
    
    // 处理缩写't（将"don't"、"Feed'st"变为"dont"、"feedst"）
    pos = strstr(str, "'t");
    while (pos != NULL) {
        // 替换't为t
        *pos = 't';
        // 删除后面的字符
        memmove(pos + 1, pos + 2, strlen(pos + 2) + 1);
        len--;
        
        pos = strstr(str, "'t");
    }
    
    // 处理连字符（将"self-substantial"变为"selfsubstantial"）
    pos = strchr(str, '-');
    while (pos != NULL) {
        // 删除连字符
        memmove(pos, pos + 1, strlen(pos + 1) + 1);
        len--;
        
        // 查找下一个连字符
        pos = strchr(str, '-');
    }

    pos = strchr(str, '<');
    while (pos != NULL) {
        // 删除连字符
        memmove(pos, pos + 1, strlen(pos + 1) + 1);
        len--;
        
        // 查找下一个连字符
        pos = strchr(str, '<');
    }

    pos = strchr(str, '>');
    while (pos != NULL) {
        // 删除连字符
        memmove(pos, pos + 1, strlen(pos + 1) + 1);
        len--;
        
        // 查找下一个连字符
        pos = strchr(str, '>');
    }

    pos = strchr(str, '.');
    while (pos != NULL) {
        // 删除连字符
        memmove(pos, pos + 1, strlen(pos + 1) + 1);
        len--;
        
        // 查找下一个连字符
        pos = strchr(str, '.');
    }

    pos = strchr(str, '*');
    while (pos != NULL) {
        // 删除连字符
        memmove(pos, pos + 1, strlen(pos + 1) + 1);
        len--;
        
        // 查找下一个连字符
        pos = strchr(str, '*');
    }

    pos = strchr(str, '(');
    while (pos != NULL) {
        // 删除连字符
        memmove(pos, pos + 1, strlen(pos + 1) + 1);
        len--;
        
        // 查找下一个连字符
        pos = strchr(str, '(');
    }

    pos = strchr(str, ')');
    while (pos != NULL) {
        // 删除连字符
        memmove(pos, pos + 1, strlen(pos + 1) + 1);
        len--;
        
        // 查找下一个连字符
        pos = strchr(str, ')');
    }

    pos = strchr(str, '"');
    while (pos != NULL) {
        // 删除连字符
        memmove(pos, pos + 1, strlen(pos + 1) + 1);
        len--;
        
        // 查找下一个连字符
        pos = strchr(str, '"');
    }

    pos = strchr(str, ':');
    while (pos != NULL) {
        // 删除连字符
        memmove(pos, pos + 1, strlen(pos + 1) + 1);
        len--;
        
        // 查找下一个连字符
        pos = strchr(str, ':');
    }

    pos = strchr(str, '/');
    while (pos != NULL) {
        // 删除连字符
        memmove(pos, pos + 1, strlen(pos + 1) + 1);
        len--;
        
        // 查找下一个连字符
        pos = strchr(str, '/');
    }
    
    pos = strchr(str, '{');
    while (pos != NULL) {
        // 删除连字符
        memmove(pos, pos + 1, strlen(pos + 1) + 1);
        len--;
        
        // 查找下一个连字符
        pos = strchr(str, '{');
    }

    pos = strchr(str, '}');
    while (pos != NULL) {
        // 删除连字符
        memmove(pos, pos + 1, strlen(pos + 1) + 1);
        len--;
        
        // 查找下一个连字符
        pos = strchr(str, '}');
    }

    pos = strchr(str, ',');
    while (pos != NULL) {
        // 删除连字符
        memmove(pos, pos + 1, strlen(pos + 1) + 1);
        len--;
        
        // 查找下一个连字符
        pos = strchr(str, ',');
    }
    
    // 处理其他撇号（如果还有）
    pos = strchr(str, '\'');
    while (pos != NULL) {
        // 删除撇号
        memmove(pos, pos + 1, strlen(pos + 1) + 1);
        len--;
        
        pos = strchr(str, '\'');
    }
}



// 提取并处理单词
void extract_words(char* line) {
    // 创建一个副本来处理
    char buffer[BUFFER_SIZE];
    strncpy(buffer, line, BUFFER_SIZE - 1);
    buffer[BUFFER_SIZE - 1] = '\0';
    
    // 标准化处理
    normalize_string(buffer);
    
    char* word = strtok(buffer, " \t\n\r\f\v.,;:!?\"()[]{}");
    
    while (word != NULL) {
        // 检查是否有字母字符
        bool has_letter = false;
        for (int i = 0; word[i]; i++) {
            if (isalpha((unsigned char)word[i])) {
                has_letter = true;
                break;
            }
        }
        
        if (has_letter) {
            printf("%s 1\n", word);
        }
        
        word = strtok(NULL, " \t\n\r\f\v.,;:!?\"()[]{}");
    }
}

int main() {
    // 设置stdout为无缓冲
    setvbuf(stdout, NULL, _IONBF, 0);
    
    char buffer[BUFFER_SIZE];
    char line[BUFFER_SIZE * 2] = "";
    
    while (1) {
        ssize_t n = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
        if (n <= 0) {
            if (n < 0 && (errno == EINTR || errno == EAGAIN)) {
                continue;
            }
            break;
        }
        
        buffer[n] = '\0';
        
        // 处理输入，可能包含多行
        char* start = buffer;
        char* end;
        
        // 处理完整行
        while ((end = strchr(start, '\n')) != NULL) {
            *end = '\0';
            
            // 将当前行添加到累积行
            if (strlen(line) + strlen(start) < sizeof(line) - 1) {
                strcat(line, start);
            }
            
            // 处理累积的行
            extract_words(line);
            
            // 重置累积行
            line[0] = '\0';
            
            start = end + 1;
        }
        
        // 将剩余部分添加到累积行
        if (*start && strlen(line) + strlen(start) < sizeof(line) - 1) {
            strcat(line, start);
        }
    }
    
    // 处理剩余的累积行
    if (line[0]) {
        extract_words(line);
    }
    
    return 0;
}