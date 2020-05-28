#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

struct token_t *tokens_buffer;

typedef enum {
    TT_NONE,
    TT_KEYWORD,
    TT_SYMBOL,
    TT_NUMBER,
    TT_OPEN_PAREN,
    TT_CLOSE_PAREN,
    TT_OPEN_CURLY,
    TT_CLOSE_CURLY,
    TT_SEMI_COLON,
    TT_COMMENT,
    TT_NEW_LINE,
    TT_WHITESPACE
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

static uint32_t s_hash_impl(
    char *buffer,
    size_t buflength) {
    uint32_t s1 = 1;
    uint32_t s2 = 0;

    for (size_t n = 0; n < buflength; n++) {
        s1 = (s1 + buffer[n]) % 65521;
        s2 = (s2 + s1) % 65521;
    }
    return (s2 << 16) | s1;
}

static uint32_t s_hash(
    char *string,
    uint32_t length) {
    return s_hash_impl(string, length - 1);
}

static string_tree_node_t *s_tree_node_create() {
    string_tree_node_t *new_node = (string_tree_node_t *)malloc(sizeof(string_tree_node_t));
    memset((void *)new_node, 0, sizeof(string_tree_node_t));
    return new_node;
}

static void s_string_data_init(
    uint32_t data, 
    char *string,
    uint32_t string_length, 
    uint32_t hash, 
    string_tree_node_data_t *node_data) {
    node_data->initialised = 1;
    node_data->data = data;
    node_data->hash = hash;
    node_data->string = string;
    node_data->string_length = string_length;
}

static void s_string_data_deinit(
    string_tree_node_data_t *node_data) {
    node_data->initialised = 0;
    node_data->data = 0;
    node_data->hash = 0;
    node_data->string = NULL;
    node_data->string_length = 0;
}

static void *s_handle_conflict(
    uint32_t current_char,
    const char *string, 
    uint32_t length,
    uint32_t hash,
    uint32_t data,
    string_tree_node_t *current,
    string_tree_node_data_t *string_data) {
    string_tree_node_data_t smaller, bigger;

    if (string_data->string_length < length) {
        s_string_data_init(string_data->data, string_data->string, string_data->string_length, string_data->hash, &smaller);
        s_string_data_init(data, (char *)string, length, hash, &bigger);
    }
    else {
        s_string_data_init(data, (char *)string, length, hash, &smaller);
        s_string_data_init(string_data->data, string_data->string, string_data->string_length, string_data->hash, &bigger);
    }

    s_string_data_deinit(string_data);
    
    string_tree_node_t *current_node = current;
    for (uint32_t i = current_char; i < smaller.string_length; i++) {
        if (smaller.string[i] == bigger.string[i]) {
            // Need to do process again
            if (i == smaller.string_length - 1) {
                // End
                // This is the smaller string's final node
                s_string_data_init(smaller.data, smaller.string, smaller.string_length, smaller.hash, &current_node->nodes[smaller.string[i]]);

                current_node->nodes[bigger.string[i]].next = s_tree_node_create();
                current_node = current_node->nodes[bigger.string[i]].next;
                s_string_data_init(bigger.data, bigger.string, bigger.string_length, bigger.hash, &current_node->nodes[bigger.string[i + 1]]);
                
                break;
            }
            
            current_node->nodes[bigger.string[i]].next = s_tree_node_create();
            current_node = current_node->nodes[bigger.string[i]].next;
        }
        else {
            // No more conflicts : can dump string in their respective character slots
            s_string_data_init(smaller.data, smaller.string, smaller.string_length, smaller.hash, &current_node->nodes[smaller.string[i]]);
            s_string_data_init(bigger.data, bigger.string, bigger.string_length, bigger.hash, &current_node->nodes[bigger.string[i]]);

            break;
        }
    }
}

static void s_register_string(
    string_tree_node_t *root, 
    const char *string, 
    uint32_t length,
    uint32_t data) {
    string_tree_node_t *current = root;
    uint32_t hash = s_hash((char *)string, length);
    string_tree_node_data_t *string_data = &current->nodes[string[0]];

    uint32_t i = 0;
    for (; i < length && current->nodes[string[i]].next; ++i) {
        string_data = &current->nodes[string[i]];
        current = string_data->next;
    }

    // The symbol already exists
    if (string_data->hash == hash) {
        printf("Symbol already exists\n");
        exit(1);
    }

    // If string has already been initialised
    if (string_data->initialised) {
        // Problem: need to move the current string to another node
        s_handle_conflict(
            i,
            string,
            length,
            hash,
            data,
            current,
            string_data);

        return;
    }

    s_string_data_init(data, string, length, hash, string_data);
}

static string_tree_node_data_t *s_traverse_tree(
    string_tree_node_t *root,
    const char *string,
    uint32_t length) {
    string_tree_node_t *current = root;
    uint32_t hash = s_hash((char *)string, length);

    for (uint32_t i = 0; i < length && current; ++i) {
        if (current->nodes[string[i]].hash == hash) {
            return &current->nodes[string[i]];
        }
        
        current = current->nodes[string[i]].next;
    }

    printf("Symbol does not exist\n");
    return NULL;
}

typedef struct {
    char *at;
    uint32_t length;
    token_type_t type;
} token_t;

static token_type_t s_determine_character_token_type(
    char c) {
    switch(c) {
        
    case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': return TT_NUMBER;
    case '(': return TT_OPEN_PAREN;
    case ')': return TT_CLOSE_PAREN;
    case '{': return TT_OPEN_CURLY;
    case '}': return TT_CLOSE_CURLY;
    case ';': return TT_SEMI_COLON;
    default: return TT_NONE;

    }
}

static token_type_t s_determine_token_type(
    char *pointer) {
    token_type_t first_character = s_determine_character_token_type(pointer[0]);
    if (first_character != TT_NONE) {
        return first_character;
    }

    /* Isn't number, bracket, curly or semi colon */
    for (uint32_t k = 0; k < sizeof(KEYWORDS) / sizeof(const char *); ++k) {
        
    }
}

static token_t s_tokenise(
    char comment_character,
    uint32_t *token_count,
    char *pointer_start) {
    token_t *tokens = tokens_buffer;
    *token_count = 0;

    char *current_whitespace_start = NULL;
    
    char *word_start = NULL;
    char *number_start = NULL;

    for (char *c = pointer_start; *c != 0; ++c) {
        token_type_t type = s_determine_character_token_type(*c);

        switch(type) {
        case TT_NEW_LINE: {
            token_t token = {};
            token.at = c;
            token.length = 1;
            token.type = TT_NEW_LINE;
            tokens[*token_count++] = token;
        } break;

        case TT_COMMENT: {
            token_t token = {};
            token.at = c;
            token.length = 1;
            token.type = TT_COMMENT;
            tokens[*token_count++] = token;
        } break;

        case TT_WHITESPACE: {
            token_t token = {};
            token.at = c;
            token.length = 1;
            token.type = TT_WHITESPACE;
            tokens[*token_count++] = token;
        } break;

        case TT_NONE: {
            if (!word_start) {
                word_start = c;
            }
        } break;

        case TT_NUMBER: {
            if (!word_start) {
                if (!number_start) {
                    number_start = c;
                }
            }
        } break;

        default: {

        } break;
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
    tokens_buffer = (token_t *)malloc(sizeof(token_t) * 1000);

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
