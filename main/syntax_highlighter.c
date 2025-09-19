#include "syntax_highlighter.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static const char *TAG = "SYNTAX_HIGHLIGHTER";

// Global syntax highlighter instance
static syntax_highlighter_t g_highlighter = {0};

// Language-specific keywords
static const char* json_keywords[] = {
    "true", "false", "null", NULL
};

static const char* python_keywords[] = {
    "and", "as", "assert", "break", "class", "continue", "def", "del", "elif", "else",
    "except", "exec", "finally", "for", "from", "global", "if", "import", "in", "is",
    "lambda", "not", "or", "pass", "print", "raise", "return", "try", "while", "with",
    "yield", "async", "await", "nonlocal", "__init__", "__main__", "self", NULL
};

static const char* html_tags[] = {
    "html", "head", "title", "body", "div", "span", "p", "a", "img", "br", "hr",
    "h1", "h2", "h3", "h4", "h5", "h6", "ul", "ol", "li", "table", "tr", "td", "th",
    "form", "input", "button", "textarea", "select", "option", "script", "style",
    "meta", "link", "nav", "header", "footer", "section", "article", "aside", NULL
};

static const char* css_properties[] = {
    "background", "background-color", "background-image", "color", "font", "font-size",
    "font-family", "font-weight", "margin", "padding", "border", "border-radius",
    "width", "height", "display", "position", "top", "left", "right", "bottom",
    "float", "clear", "text-align", "text-decoration", "line-height", "opacity",
    "z-index", "overflow", "visibility", "cursor", "transition", "transform", NULL
};

static const char* js_keywords[] = {
    "abstract", "arguments", "boolean", "break", "byte", "case", "catch", "char",
    "class", "const", "continue", "debugger", "default", "delete", "do", "double",
    "else", "enum", "eval", "export", "extends", "false", "final", "finally", "float",
    "for", "function", "goto", "if", "implements", "import", "in", "instanceof", "int",
    "interface", "let", "long", "native", "new", "null", "package", "private", "protected",
    "public", "return", "short", "static", "super", "switch", "synchronized", "this",
    "throw", "throws", "transient", "true", "try", "typeof", "var", "void", "volatile",
    "while", "with", "yield", "async", "await", NULL
};

// Forward declarations
static bool is_keyword(const char* word, const char** keyword_list);
static bool parse_tokens(const char* text, text_file_type_t file_type);
static void add_token(uint16_t start, uint16_t length, syntax_token_type_t type);
static void parse_json(const char* text);
static void parse_python(const char* text);
static void parse_html(const char* text);
static void parse_css(const char* text);
static void parse_javascript(const char* text);
static bool is_word_char(char c);
static bool is_number_char(char c);
static void init_token_styles(void);

bool syntax_highlighter_init(void) {
    if (g_highlighter.styles_initialized) {
        return true; // Already initialized
    }

    memset(&g_highlighter, 0, sizeof(syntax_highlighter_t));

    // Initialize token storage
    g_highlighter.token_capacity = 256; // Start with capacity for 256 tokens
    g_highlighter.tokens = malloc(g_highlighter.token_capacity * sizeof(syntax_token_t));
    if (!g_highlighter.tokens) {
        ESP_LOGE(TAG, "Failed to allocate memory for tokens");
        return false;
    }

    // Initialize styles
    init_token_styles();

    g_highlighter.enabled = true;
    g_highlighter.file_type = TEXT_FILE_UNKNOWN;
    g_highlighter.token_count = 0;
    g_highlighter.styles_initialized = true;

    ESP_LOGI(TAG, "Syntax highlighter initialized");
    return true;
}

void syntax_highlighter_destroy(void) {
    if (g_highlighter.tokens) {
        free(g_highlighter.tokens);
        g_highlighter.tokens = NULL;
    }

    // Clean up styles
    if (g_highlighter.styles_initialized) {
        for (int i = 0; i < TOKEN_MAX; i++) {
            lv_style_reset(&g_highlighter.token_styles[i]);
        }
        g_highlighter.styles_initialized = false;
    }

    memset(&g_highlighter, 0, sizeof(syntax_highlighter_t));
    ESP_LOGI(TAG, "Syntax highlighter destroyed");
}

void syntax_highlighter_set_enabled(bool enabled) {
    g_highlighter.enabled = enabled;
    ESP_LOGI(TAG, "Syntax highlighting %s", enabled ? "enabled" : "disabled");
}

bool syntax_highlighter_is_enabled(void) {
    return g_highlighter.enabled && g_highlighter.styles_initialized;
}

void syntax_highlighter_set_file_type(text_file_type_t file_type) {
    g_highlighter.file_type = file_type;
    ESP_LOGI(TAG, "File type set to: %s", syntax_highlighter_get_file_type_name(file_type));
}

bool syntax_highlighter_apply(lv_obj_t *textarea, bool force_refresh) {
    if (!textarea || !g_highlighter.enabled || !g_highlighter.styles_initialized) {
        return false;
    }

    if (!syntax_highlighter_supports_file_type(g_highlighter.file_type)) {
        return false;
    }

    const char *text = lv_textarea_get_text(textarea);
    if (!text || strlen(text) == 0) {
        return true;
    }

    // Parse tokens for the current file type
    bool parse_success = parse_tokens(text, g_highlighter.file_type);
    if (!parse_success) {
        ESP_LOGW(TAG, "Failed to parse tokens for file type %d", g_highlighter.file_type);
        return false;
    }

    // Note: LVGL 9.3.0 textarea doesn't support rich text styling directly
    // We would need to use a more complex approach with multiple label objects
    // or implement a custom rich text rendering system.
    // For now, we'll log the highlighting information and set basic styling.

    ESP_LOGI(TAG, "Parsed %d tokens for highlighting", g_highlighter.token_count);

    // Apply basic dark theme styling to the textarea
    lv_obj_set_style_bg_color(textarea, SYNTAX_COLOR_BACKGROUND, 0);
    lv_obj_set_style_text_color(textarea, SYNTAX_COLOR_TEXT_DEFAULT, 0);

    return true;
}

void syntax_highlighter_clear(lv_obj_t *textarea) {
    if (!textarea) return;

    // Reset to default styling
    lv_obj_set_style_bg_color(textarea, lv_color_hex(0x1e1e1e), 0);
    lv_obj_set_style_text_color(textarea, lv_color_hex(0xf8f8f2), 0);

    g_highlighter.token_count = 0;
}

lv_color_t syntax_highlighter_get_token_color(syntax_token_type_t token_type) {
    switch (token_type) {
        case TOKEN_KEYWORD: return SYNTAX_COLOR_KEYWORD;
        case TOKEN_STRING: return SYNTAX_COLOR_STRING;
        case TOKEN_COMMENT: return SYNTAX_COLOR_COMMENT;
        case TOKEN_NUMBER: return SYNTAX_COLOR_NUMBER;
        case TOKEN_FUNCTION: return SYNTAX_COLOR_FUNCTION;
        case TOKEN_TAG: return SYNTAX_COLOR_TAG;
        case TOKEN_ATTRIBUTE: return SYNTAX_COLOR_ATTRIBUTE;
        case TOKEN_PROPERTY: return SYNTAX_COLOR_PROPERTY;
        case TOKEN_PUNCTUATION: return SYNTAX_COLOR_PUNCTUATION;
        case TOKEN_OPERATOR: return SYNTAX_COLOR_OPERATOR;
        default: return SYNTAX_COLOR_TEXT_DEFAULT;
    }
}

bool syntax_highlighter_supports_file_type(text_file_type_t file_type) {
    switch (file_type) {
        case TEXT_FILE_JSON:
        case TEXT_FILE_PY:
        case TEXT_FILE_HTML:
        case TEXT_FILE_CSS:
        case TEXT_FILE_JS:
            return true;
        default:
            return false;
    }
}

const char* syntax_highlighter_get_file_type_name(text_file_type_t file_type) {
    switch (file_type) {
        case TEXT_FILE_TXT: return "Plain Text";
        case TEXT_FILE_JSON: return "JSON";
        case TEXT_FILE_PY: return "Python";
        case TEXT_FILE_HTML: return "HTML";
        case TEXT_FILE_CSS: return "CSS";
        case TEXT_FILE_JS: return "JavaScript";
        case TEXT_FILE_LOG: return "Log";
        case TEXT_FILE_CONFIG: return "Config";
        default: return "Unknown";
    }
}

// Private functions

static void init_token_styles(void) {
    for (int i = 0; i < TOKEN_MAX; i++) {
        lv_style_init(&g_highlighter.token_styles[i]);
        lv_style_set_text_color(&g_highlighter.token_styles[i],
                               syntax_highlighter_get_token_color((syntax_token_type_t)i));
    }
}

static bool is_keyword(const char* word, const char** keyword_list) {
    if (!word || !keyword_list) return false;

    for (int i = 0; keyword_list[i] != NULL; i++) {
        if (strcmp(word, keyword_list[i]) == 0) {
            return true;
        }
    }
    return false;
}

static bool parse_tokens(const char* text, text_file_type_t file_type) {
    if (!text) return false;

    // Reset token count
    g_highlighter.token_count = 0;

    switch (file_type) {
        case TEXT_FILE_JSON:
            parse_json(text);
            break;
        case TEXT_FILE_PY:
            parse_python(text);
            break;
        case TEXT_FILE_HTML:
            parse_html(text);
            break;
        case TEXT_FILE_CSS:
            parse_css(text);
            break;
        case TEXT_FILE_JS:
            parse_javascript(text);
            break;
        default:
            return false;
    }

    return true;
}

static void add_token(uint16_t start, uint16_t length, syntax_token_type_t type) {
    if (g_highlighter.token_count >= g_highlighter.token_capacity) {
        // Expand token array if needed
        g_highlighter.token_capacity *= 2;
        syntax_token_t *new_tokens = realloc(g_highlighter.tokens,
                                            g_highlighter.token_capacity * sizeof(syntax_token_t));
        if (!new_tokens) {
            ESP_LOGE(TAG, "Failed to expand token array");
            return;
        }
        g_highlighter.tokens = new_tokens;
    }

    syntax_token_t *token = &g_highlighter.tokens[g_highlighter.token_count++];
    token->start_pos = start;
    token->length = length;
    token->type = type;
    token->color = syntax_highlighter_get_token_color(type);
}

static void parse_json(const char* text) {
    int len = strlen(text);
    int i = 0;

    while (i < len) {
        char c = text[i];

        // Skip whitespace
        if (isspace(c)) {
            i++;
            continue;
        }

        // String literals
        if (c == '"') {
            int start = i;
            i++; // Skip opening quote
            while (i < len && text[i] != '"') {
                if (text[i] == '\\') i++; // Skip escaped character
                i++;
            }
            if (i < len) i++; // Skip closing quote
            add_token(start, i - start, TOKEN_STRING);
            continue;
        }

        // Numbers
        if (isdigit((unsigned char)c) || (c == '-' && i + 1 < len && isdigit((unsigned char)text[i + 1]))) {
            int start = i;
            if (c == '-') i++;
            while (i < len && is_number_char(text[i])) i++;
            add_token(start, i - start, TOKEN_NUMBER);
            continue;
        }

        // Keywords and identifiers
        if (isalpha((unsigned char)c)) {
            int start = i;
            while (i < len && is_word_char(text[i])) i++;

            char word[64];
            int word_len = i - start;
            if (word_len < (int)sizeof(word) - 1) {
                strncpy(word, &text[start], word_len);
                word[word_len] = '\0';

                if (is_keyword(word, json_keywords)) {
                    add_token(start, word_len, TOKEN_KEYWORD);
                }
            }
            continue;
        }

        // Punctuation
        if (strchr("{}[]:,", c)) {
            add_token(i, 1, TOKEN_PUNCTUATION);
        }

        i++;
    }
}

static void parse_python(const char* text) {
    int len = strlen(text);
    int i = 0;

    while (i < len) {
        char c = text[i];

        // Skip whitespace
        if (isspace(c)) {
            i++;
            continue;
        }

        // Comments
        if (c == '#') {
            int start = i;
            while (i < len && text[i] != '\n') i++;
            add_token(start, i - start, TOKEN_COMMENT);
            continue;
        }

        // String literals (single and double quotes)
        if (c == '"' || c == '\'') {
            char quote = c;
            int start = i;
            i++; // Skip opening quote

            // Handle triple quotes
            bool triple_quote = false;
            if (i + 1 < len && text[i] == quote && text[i + 1] == quote) {
                triple_quote = true;
                i += 2;
            }

            while (i < len) {
                if (triple_quote) {
                    if (i + 2 < len && text[i] == quote && text[i + 1] == quote && text[i + 2] == quote) {
                        i += 3;
                        break;
                    }
                } else {
                    if (text[i] == quote) {
                        i++;
                        break;
                    }
                    if (text[i] == '\\') i++; // Skip escaped character
                }
                i++;
            }
            add_token(start, i - start, TOKEN_STRING);
            continue;
        }

        // Numbers
        if (isdigit((unsigned char)c)) {
            int start = i;
            while (i < len && is_number_char(text[i])) i++;
            add_token(start, i - start, TOKEN_NUMBER);
            continue;
        }

        // Keywords and identifiers
        if (isalpha((unsigned char)c) || c == '_') {
            int start = i;
            while (i < len && is_word_char(text[i])) i++;

            char word[64];
            int word_len = i - start;
            if (word_len > 0 && word_len < (int)sizeof(word) - 1) {
                strncpy(word, &text[start], word_len);
                word[word_len] = '\0';

                if (is_keyword(word, python_keywords)) {
                    add_token(start, word_len, TOKEN_KEYWORD);
                } else if (i < len && text[i] == '(') {
                    // Function call
                    add_token(start, word_len, TOKEN_FUNCTION);
                }
            }
            continue;
        }

        // Operators
        if (strchr("+-*/%=<>!&|^~", c)) {
            add_token(i, 1, TOKEN_OPERATOR);
        }
        // Punctuation
        else if (strchr("()[]{}:,;.", c)) {
            add_token(i, 1, TOKEN_PUNCTUATION);
        }

        i++;
    }
}

static void parse_html(const char* text) {
    int len = strlen(text);
    int i = 0;

    while (i < len) {
        char c = text[i];

        // Skip whitespace
        if (isspace(c)) {
            i++;
            continue;
        }

        // HTML comments
        if (i + 3 < len && strncmp(&text[i], "<!--", 4) == 0) {
            int start = i;
            i += 4;
            while (i + 2 < len && strncmp(&text[i], "-->", 3) != 0) i++;
            if (i + 2 < len) i += 3;
            add_token(start, i - start, TOKEN_COMMENT);
            continue;
        }

        // HTML tags
        if (c == '<') {
            int start = i;
            i++; // Skip <

            // Skip closing tag indicator
            if (i < len && text[i] == '/') {
                i++;
            }

            // Parse tag name
            int tag_start = i;
            while (i < len && is_word_char(text[i])) i++;

            if (i > tag_start) {
                char tag[32];
                int tag_len = i - tag_start;
                if (tag_len < sizeof(tag)) {
                    strncpy(tag, &text[tag_start], tag_len);
                    tag[tag_len] = '\0';

                    if (is_keyword(tag, html_tags)) {
                        add_token(tag_start, tag_len, TOKEN_TAG);
                    }
                }
            }

            // Parse attributes
            while (i < len && text[i] != '>') {
                if (isalpha((unsigned char)text[i])) {
                    int attr_start = i;
                    while (i < len && (is_word_char(text[i]) || text[i] == '-')) i++;
                    add_token(attr_start, i - attr_start, TOKEN_ATTRIBUTE);

                    // Skip whitespace
                    while (i < len && isspace((unsigned char)text[i])) i++;

                    // Check for attribute value
                    if (i < len && text[i] == '=') {
                        i++; // Skip =
                        while (i < len && isspace((unsigned char)text[i])) i++;

                        if (i < len && (text[i] == '"' || text[i] == '\'')) {
                            char quote = text[i];
                            int value_start = i;
                            i++; // Skip opening quote
                            while (i < len && text[i] != quote) i++;
                            if (i < len) i++; // Skip closing quote
                            add_token(value_start, i - value_start, TOKEN_STRING);
                        }
                    }
                }
                i++;
            }

            if (i < len) i++; // Skip >
            add_token(start, 1, TOKEN_PUNCTUATION); // For <
            add_token(i - 1, 1, TOKEN_PUNCTUATION); // For >
            continue;
        }

        i++;
    }
}

static void parse_css(const char* text) {
    int len = strlen(text);
    int i = 0;

    while (i < len) {
        char c = text[i];

        // Skip whitespace
        if (isspace(c)) {
            i++;
            continue;
        }

        // CSS comments
        if (i + 1 < len && text[i] == '/' && text[i + 1] == '*') {
            int start = i;
            i += 2;
            while (i + 1 < len && !(text[i] == '*' && text[i + 1] == '/')) i++;
            if (i + 1 < len) i += 2;
            add_token(start, i - start, TOKEN_COMMENT);
            continue;
        }

        // String literals
        if (c == '"' || c == '\'') {
            char quote = c;
            int start = i;
            i++; // Skip opening quote
            while (i < len && text[i] != quote) {
                if (text[i] == '\\') i++; // Skip escaped character
                i++;
            }
            if (i < len) i++; // Skip closing quote
            add_token(start, i - start, TOKEN_STRING);
            continue;
        }

        // CSS properties
        if (isalpha((unsigned char)c) || c == '-') {
            int start = i;
            while (i < len && (is_word_char(text[i]) || text[i] == '-')) i++;

            char prop[64];
            int prop_len = i - start;
            if (prop_len > 0 && prop_len < (int)sizeof(prop) - 1) {
                strncpy(prop, &text[start], prop_len);
                prop[prop_len] = '\0';

                if (is_keyword(prop, css_properties)) {
                    add_token(start, prop_len, TOKEN_PROPERTY);
                }
            }
            continue;
        }

        // Numbers (including units like px, em, %)
        if (isdigit((unsigned char)c)) {
            int start = i;
            while (i < len && (isdigit((unsigned char)text[i]) || text[i] == '.' || text[i] == '%')) i++;
            // Include units
            while (i < len && isalpha((unsigned char)text[i])) i++;
            add_token(start, i - start, TOKEN_NUMBER);
            continue;
        }

        // Punctuation
        if (strchr("{}:;,", c)) {
            add_token(i, 1, TOKEN_PUNCTUATION);
        }

        i++;
    }
}

static void parse_javascript(const char* text) {
    int len = strlen(text);
    int i = 0;

    while (i < len) {
        char c = text[i];

        // Skip whitespace
        if (isspace(c)) {
            i++;
            continue;
        }

        // Single line comments
        if (i + 1 < len && text[i] == '/' && text[i + 1] == '/') {
            int start = i;
            while (i < len && text[i] != '\n') i++;
            add_token(start, i - start, TOKEN_COMMENT);
            continue;
        }

        // Multi-line comments
        if (i + 1 < len && text[i] == '/' && text[i + 1] == '*') {
            int start = i;
            i += 2;
            while (i + 1 < len && !(text[i] == '*' && text[i + 1] == '/')) i++;
            if (i + 1 < len) i += 2;
            add_token(start, i - start, TOKEN_COMMENT);
            continue;
        }

        // String literals
        if (c == '"' || c == '\'' || c == '`') {
            char quote = c;
            int start = i;
            i++; // Skip opening quote
            while (i < len && text[i] != quote) {
                if (text[i] == '\\') i++; // Skip escaped character
                i++;
            }
            if (i < len) i++; // Skip closing quote
            add_token(start, i - start, TOKEN_STRING);
            continue;
        }

        // Numbers
        if (isdigit((unsigned char)c)) {
            int start = i;
            while (i < len && is_number_char(text[i])) i++;
            add_token(start, i - start, TOKEN_NUMBER);
            continue;
        }

        // Keywords and identifiers
        if (isalpha((unsigned char)c) || c == '_' || c == '$') {
            int start = i;
            while (i < len && (is_word_char(text[i]) || text[i] == '$')) i++;

            char word[64];
            int word_len = i - start;
            if (word_len > 0 && word_len < (int)sizeof(word) - 1) {
                strncpy(word, &text[start], word_len);
                word[word_len] = '\0';

                if (is_keyword(word, js_keywords)) {
                    add_token(start, word_len, TOKEN_KEYWORD);
                } else if (i < len && text[i] == '(') {
                    // Function call
                    add_token(start, word_len, TOKEN_FUNCTION);
                }
            }
            continue;
        }

        // Operators
        if (strchr("+-*/%=<>!&|^~?:", c)) {
            add_token(i, 1, TOKEN_OPERATOR);
        }
        // Punctuation
        else if (strchr("()[]{}.,;", c)) {
            add_token(i, 1, TOKEN_PUNCTUATION);
        }

        i++;
    }
}

static bool is_word_char(char c) {
    return isalnum((unsigned char)c) || c == '_';
}

static bool is_number_char(char c) {
    return isdigit((unsigned char)c) || c == '.' || c == 'e' || c == 'E' || c == '+' || c == '-';
}