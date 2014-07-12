// vim: set ts=8 sts=2 sw=2 tw=99 et:
#ifndef _sourcepawn_compiler_token_stream_h_
#define _sourcepawn_compiler_token_stream_h_

typedef struct {
  int line;
  int col;
} token_pos_t;

typedef struct {
  int id;
  int value;
  char str[sLINEMAX + 1];
  size_t len;
  token_pos_t start;
  token_pos_t end;
} full_token_t;

#define MAX_TOKEN_DEPTH 4

typedef struct {
  // Total number of tokens parsed.
  int num_tokens;

  // Number of tokens that we've rewound back to.
  int depth;

  // Most recently fetched token.
  int cursor;

  // Circular token buffer.
  full_token_t tokens[MAX_TOKEN_DEPTH];
} token_buffer_t;

#endif // _sourcepawn_compiler_token_stream_h_
