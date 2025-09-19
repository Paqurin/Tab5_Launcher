#ifndef SYNTAX_HIGHLIGHTER_H
#define SYNTAX_HIGHLIGHTER_H

#include "lvgl.h"
#include "gui_screen_text_editor.h"

// Syntax highlighting color scheme (dark theme professional colors)
#define SYNTAX_COLOR_BACKGROUND    lv_color_hex(0x1e1e1e)  // Dark background
#define SYNTAX_COLOR_TEXT_DEFAULT  lv_color_hex(0xf8f8f2)  // Light text (default)
#define SYNTAX_COLOR_KEYWORD       lv_color_hex(0xff79c6)  // Pink for keywords
#define SYNTAX_COLOR_STRING        lv_color_hex(0xf1fa8c)  // Yellow for strings
#define SYNTAX_COLOR_COMMENT       lv_color_hex(0x6272a4)  // Blue-gray for comments
#define SYNTAX_COLOR_NUMBER        lv_color_hex(0xbd93f9)  // Purple for numbers
#define SYNTAX_COLOR_FUNCTION      lv_color_hex(0x50fa7b)  // Green for functions
#define SYNTAX_COLOR_TAG           lv_color_hex(0xff5555)  // Red for HTML tags
#define SYNTAX_COLOR_ATTRIBUTE     lv_color_hex(0xffb86c)  // Orange for attributes
#define SYNTAX_COLOR_PROPERTY      lv_color_hex(0x8be9fd)  // Cyan for CSS properties
#define SYNTAX_COLOR_PUNCTUATION  lv_color_hex(0xf8f8f2)  // Light for punctuation
#define SYNTAX_COLOR_OPERATOR      lv_color_hex(0xff79c6)  // Pink for operators

// Token types for syntax highlighting
typedef enum {
    TOKEN_DEFAULT = 0,
    TOKEN_KEYWORD,
    TOKEN_STRING,
    TOKEN_COMMENT,
    TOKEN_NUMBER,
    TOKEN_FUNCTION,
    TOKEN_TAG,
    TOKEN_ATTRIBUTE,
    TOKEN_PROPERTY,
    TOKEN_PUNCTUATION,
    TOKEN_OPERATOR,
    TOKEN_MAX
} syntax_token_type_t;

// Token structure
typedef struct {
    uint16_t start_pos;
    uint16_t length;
    syntax_token_type_t type;
    lv_color_t color;
} syntax_token_t;

// Syntax highlighting state
typedef struct {
    bool enabled;
    text_file_type_t file_type;
    syntax_token_t *tokens;
    uint16_t token_count;
    uint16_t token_capacity;
    lv_style_t token_styles[TOKEN_MAX];
    bool styles_initialized;
} syntax_highlighter_t;

/**
 * @brief Initialize syntax highlighter
 * @return true on success
 */
bool syntax_highlighter_init(void);

/**
 * @brief Destroy syntax highlighter and free resources
 */
void syntax_highlighter_destroy(void);

/**
 * @brief Enable/disable syntax highlighting
 * @param enabled true to enable, false to disable
 */
void syntax_highlighter_set_enabled(bool enabled);

/**
 * @brief Check if syntax highlighting is enabled
 * @return true if enabled
 */
bool syntax_highlighter_is_enabled(void);

/**
 * @brief Set file type for syntax highlighting
 * @param file_type Type of file to highlight
 */
void syntax_highlighter_set_file_type(text_file_type_t file_type);

/**
 * @brief Apply syntax highlighting to text area
 * @param textarea LVGL textarea object to highlight
 * @param force_refresh Force full re-highlighting
 * @return true on success
 */
bool syntax_highlighter_apply(lv_obj_t *textarea, bool force_refresh);

/**
 * @brief Clear all syntax highlighting from text area
 * @param textarea LVGL textarea object to clear
 */
void syntax_highlighter_clear(lv_obj_t *textarea);

/**
 * @brief Get color for token type
 * @param token_type Type of token
 * @return Color for the token type
 */
lv_color_t syntax_highlighter_get_token_color(syntax_token_type_t token_type);

/**
 * @brief Check if file type supports syntax highlighting
 * @param file_type Type of file to check
 * @return true if highlighting is supported
 */
bool syntax_highlighter_supports_file_type(text_file_type_t file_type);

/**
 * @brief Get human-readable name for file type
 * @param file_type Type of file
 * @return String name of file type
 */
const char* syntax_highlighter_get_file_type_name(text_file_type_t file_type);

#endif // SYNTAX_HIGHLIGHTER_H