#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    QUOTE_NONE,
    QUOTE_HAPPY,
    QUOTE_NEUTRAL,
    QUOTE_MAD,
    QUOTE_MISC,
    QUOTE_WELCOME
} DicemoQuoteType;

// Declare the global
extern DicemoQuoteType pending_quote_type;

const char* get_random_quote(DicemoQuoteType type);

#ifdef __cplusplus
}
#endif
