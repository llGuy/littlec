#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

typedef enum {
    TT_KEYWORD,
    TT_SYMBOL,
    TT_NUMBER,
    TT_OPEN_PAREN,
    TT_CLOSE_PAREN,
    TT_OPEN_CURLY,
    TT_CLOSE_CURLY,
    TT_SEMI_COLON
} token_type_t;

typedef enum {
    K_INT,
    K_RETURN
} keyword_t;

static const char *KEYWORDS[] = {
    "int",
    "return"
};

typedef struct {
    struct {
        uint64_t data : 16;
        uint64_t string_length : 16;
        uint64_t initialised : 1;
        uint64_t hash : 32;
    };
    char *string;

    struct string_tree_node_t *next;
} string_tree_node_data_t;

typedef struct {
    string_tree_node_data_t nodes[127];
} string_tree_node_t;

static uint32_t s_hash_impl(char *buffer, size_t buflength) {
    uint32_t s1 = 1;
    uint32_t s2 = 0;

    for (size_t n = 0; n < buflength; n++) {
        s1 = (s1 + buffer[n]) % 65521;
        s2 = (s2 + s1) % 65521;
    }
    return (s2 << 16) | s1;
}

static uint32_t s_hash(const char *string, uint32_t length) {
    return s_hash_impl(string, length - 1);
}

static string_tree_node_t *s_tree_node_create() {
    string_tree_node_t *new_node = (string_tree_node_t *)malloc(sizeof(string_tree_node_t));
    memset(new_node, 0, sizeof(string_tree_node_t));
    return new_node;
}

static void *s_handle_conflict(
    uint32_t current_char,
    const char *string, 
    uint32_t length,
    uint32_t hash,
    uint32_t data,
    string_tree_node_data_t *string_data) {
    const char *other_string = string_data->string;
    uint32_t other_length = string_data->string_length;
    uint32_t other_hash = string_data->hash;
    uint32_t other_data = string_data->data;

    if (current_char == length - 1) {
        // This string belongs here
        // Need to swap
        string_data->string = string;
        string_data->string_length = length;
        string_data->data = data;
        string_data->hash = hash;

        string_data->next = s_tree_node_create();
        
        string_tree_node_t *next_node = (string_tree_node_t *)(string_data->next);
        string_tree_node_data_t *next_char = &next_node->nodes[other_string[current_char]];
        next_char->initialised = 1;
        next_char->data = other_data;
        next_char->string = other_string;
        next_char->string_length = other_length;
        next_char->hash = other_hash;

        return;
    }
    else if (current_char == other_length - 1) {
        // Other string belongs here - don't change
        string_data->next = s_tree_node_create();

        string_tree_node_t *next_node = (string_tree_node_t *)(string_data->next);
        string_tree_node_data_t *next_char = &next_node->nodes[string[current_char]];
        next_char->initialised = 1;
        next_char->data = data;
        next_char->string = string;
        next_char->string_length = length;
        next_char->hash = hash;

        return;
    }
    else {
        // Both need to be moved
        string_data->next = s_tree_node_create();
        string_data->initialised = 0;
        string_data->hash = 0;
        string_data->string = NULL;
        string_data->string_length = 0;
        string_data->data = 0;

        const char *smaller_string, *bigger_string;
        uint32_t smaller_length, bigger_length;
        uint32_t smaller_hash, bigger_hash;
        uint32_t smaller_data, bigger_data;

        if (other_length < length) {
            smaller_string = other_string, bigger_string = string;
            smaller_length = other_length, bigger_length = length;
            smaller_hash = other_hash, bigger_hash = hash;
            smaller_data = other_data, bigger_data = data;
        }
        else {
            smaller_string = string, bigger_string = other_string;
            smaller_length = length, bigger_length = other_length;
            smaller_hash = hash, bigger_hash = other_hash;
            smaller_data = data, bigger_data = other_data;
        }

        string_tree_node_t *current_node = string_data->next;
        for (uint32_t i = current_char + 1; i < smaller_length; i++) {
            if (other_string[i] == string[i]) {
                // Need to do process again
                if (i == smaller_length - 1) {
                    // End
                    // This is the smaller string's final node
                    current_node->nodes[smaller_string[i]].initialised = 1;
                    current_node->nodes[smaller_string[i]].string = smaller_string;
                    current_node->nodes[smaller_string[i]].data = smaller_data;
                    current_node->nodes[smaller_string[i]].string_length = smaller_length;
                    current_node->nodes[smaller_string[i]].hash = smaller_hash;

                    current_node->nodes[bigger_string[i]].next = s_tree_node_create();
                    current_node = current_node->nodes[bigger_string[i]].next;
                    current_node->nodes[bigger_string[i + 1]].initialised = 1;
                    current_node->nodes[bigger_string[i + 1]].string = bigger_string;
                    current_node->nodes[bigger_string[i + 1]].data = bigger_data;
                    current_node->nodes[bigger_string[i + 1]].string_length = bigger_length;
                    current_node->nodes[bigger_string[i + 1]].hash = bigger_hash;

                    break;
                }

                current_node->nodes[bigger_string[i]].next = s_tree_node_create();
                current_node = current_node->nodes[other_string[i]].next;
            }
            else {
                // No more conflicts : can dump string in their respective character slots
                current_node->nodes[smaller_string[i]].initialised = 1;
                current_node->nodes[smaller_string[i]].data = smaller_data;
                current_node->nodes[smaller_string[i]].hash = smaller_hash;
                current_node->nodes[smaller_string[i]].string = smaller_string;
                current_node->nodes[smaller_string[i]].string_length = smaller_length;

                current_node->nodes[bigger_string[i]].initialised = 1;
                current_node->nodes[bigger_string[i]].data = bigger_data;
                current_node->nodes[bigger_string[i]].hash = bigger_hash;
                current_node->nodes[bigger_string[i]].string = bigger_string;
                current_node->nodes[bigger_string[i]].string_length = bigger_length;

                break;
            }
        }
    }
}

static void *s_register_string(
    string_tree_node_t *root, 
    const char *string, 
    uint32_t length,
    uint32_t data) {
    string_tree_node_t *current = root;
    uint32_t hash = s_hash(string, length);
    for (uint32_t i = 0; i < length; ++i) {
        string_tree_node_data_t *string_data = &current->nodes[string[i]];

        if (string_data->next) {
            current = string_data->next;
        }
        else {
            if (string_data->initialised) {
                if (string_data->hash == hash) {
                    printf("Symbol already exists\n");
                    exit(1);
                }

                // Problem: need to move the current string to another node
                s_handle_conflict(
                    i,
                    string,
                    length,
                    hash,
                    data,
                    string_data);

                return;
            }

            string_data->initialised = 1;
            string_data->hash = hash;
            string_data->data = data;
            string_data->string = string;
            string_data->string_length = length;

            return;
        }
    }

    string_tree_node_data_t *string_data = &current->nodes[string[length - 1]];

    string_data->hash = hash;
    string_data->initialised = 1;
    string_data->data = data;
    string_data->string = string;
    string_data->string_length = length;
}

static string_tree_node_data_t *s_traverse_tree(
    string_tree_node_t *root,
    const char *string,
    uint32_t length) {
    string_tree_node_t *current = root;
    uint32_t hash = s_hash(string, length);
    for (uint32_t i = 0; i < length; ++i) {
        if (current->nodes[string[i]].hash == hash) {
            return &current->nodes[string[i]];
        }
        else if (current->nodes[string[i]].next) {
            current = current->nodes[string[i]].next;
        }
        else {
            printf("Unregistered symbol\n");
            return NULL;
        }
    }

    if (current->nodes[string[length - 1]].initialised) {
        return &current->nodes[string[length - 1]];
    }
    else {
        printf("Unregistered symbol\n");
        return NULL;
    }
}

typedef struct {
    char *at;
    uint32_t length;
    token_type_t type;
} token_t;

static token_type_t s_determine_token_type(
    char *pointer) {
    switch(pointer[0]) {
        
    case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': return TT_NUMBER;
    case '(': return TT_OPEN_PAREN;
    case ')': return TT_CLOSE_PAREN;
    case '{': return TT_OPEN_CURLY;
    case '}': return TT_CLOSE_CURLY;
    case ';': return TT_SEMI_COLON;
    default: break;

    }

    /* Isn't number, bracket, curly or semi colon */
    for (uint32_t k = 0; k < sizeof(KEYWORDS) / sizeof(const char *); ++k) {
        if (KEYWORDS[k][0] == pointer[0]) {
            
        }
    }
}

int32_t lc_do_file(
    const char *path) {
    FILE *file = fopen(path, "r");
    fseek(file, 0L, SEEK_END);
    uint32_t file_size = ftell(file);
    rewind(file);

    char *code = (char *)malloc(sizeof(char) * file_size);
    
    fgets(code, file_size, file);

    uint32_t code_length = strlen(code);
    for (uint32_t i = 0; i < code_length; ++i) {
        putchar(code[i]);
    }

    return 0;
}

int32_t main(
    int32_t argc,
    char *argv[]) {
//    lc_do_file("../test/t0.c");

    string_tree_node_t *root = s_tree_node_create();

    const char *integer = "integer";

    s_register_string(root, integer, strlen(integer), 42);
    s_register_string(root, KEYWORDS[0], strlen(KEYWORDS[0]), K_INT);
    s_register_string(root, KEYWORDS[1], strlen(KEYWORDS[1]), K_RETURN);

    printf("%d", sizeof(string_tree_node_t));

    string_tree_node_data_t *data_int = s_traverse_tree(root, KEYWORDS[0], strlen(KEYWORDS[0]));
    string_tree_node_data_t *data_return = s_traverse_tree(root, KEYWORDS[1], strlen(KEYWORDS[1]));
    string_tree_node_data_t *data_integer = s_traverse_tree(root, integer, strlen(integer));
}
