#include <stdio.h>
#include <stdint.h>
#include <malloc.h>

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
        uint32_t data : 31;
        uint32_t initialized : 1;
    };
    struct string_tree_node_t *next;
} string_tree_node_data_t;

typedef struct {
    string_tree_node_data_t nodes[127];
} string_tree_node_t;

static string_tree_node_t *s_tree_node_create() {
    string_tree_node_t *new_node = (string_tree_node_t *)malloc(sizeof(string_tree_node_t));
    memset(new_node, 0, sizeof(string_tree_node_t));
    return new_node;
}

static void *s_register_string(
    string_tree_node_t *root, 
    const char *string, 
    uint32_t length,
    uint32_t data) {
    string_tree_node_t *current = root;
    for (uint32_t i = 0; i < length; ++i) {
        if (current->nodes[string[i]].next) {
            current = current->nodes[string[i]].next;
        }
        else {
            // Need to create new node
            current->nodes[string[i]].next = s_tree_node_create();
            current = current->nodes[string[i]].next;
        }
    }
    current->nodes[string[length - 1]].initialized = 1;
    current->nodes[string[length - 1]].data = data;
}

static string_tree_node_data_t *s_traverse_tree(
    string_tree_node_t *root,
    const char *string,
    uint32_t length) {
    string_tree_node_t *current = root;
    for (uint32_t i = 0; i < length; ++i) {
        if (current->nodes[string[i]].next) {
            current = current->nodes[string[i]].next;
        }
        else {
            printf("Unregistered symbol\n");
            return NULL;
        }
    }

    if (current->nodes[string[length - 1]].initialized) {
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
