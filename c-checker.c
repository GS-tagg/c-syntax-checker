
/*
 * TODO:
 *
 * [ ] Handle character literals ('a', '\n', etc.)
 * [ ] Validate escape sequences (\n, \t, \\, \", etc.)
 * [ ] Detect empty character literals ('')
 * [ ] Detect unterminated character literals
 * [ ] Detect invalid characters
 * [ ] Detect invalid numeric literals
 * [ ] Handle preprocessor directives (#include, #define, etc.)
 * [ ] Handle line continuation (\)
 *
 * [ ] Detect bracket stack overflow
 * [ ] Improve mismatched bracket diagnostics
 * [ ] Detect missing semicolons
 * [ ] Detect empty declarations
 * [ ] Detect empty initialisers
 * [ ] Detect missing operands
 * [ ] Detect missing conditions
 * [ ] Detect malformed function declarations
 * [ ] Detect malformed expressions
 *
 * [ ] Implement proper tokenisation
 * [ ] Track token types and source offsets
 * [ ] Recognise operators (++, --, ->, &&, ||, etc.)
 * [ ] Recognise identifiers and keywords
 * [ ] Recognise numeric constants and suffixes
 *
 * [ ] Implement basic parsing
 * [ ] Validate declarations
 * [ ] Validate statements
 * [ ] Validate function definitions
 * [ ] Validate expressions
 *
 * [ ] Track variable declarations
 * [ ] Detect duplicate declarations
 * [ ] Detect undeclared identifiers
 * [ ] Track variable scopes
 * [ ] Detect unused variables
 * [ ] Detect uninitialised variables
 * [ ] Validate function arguments
 * [ ] Validate return types
 * [ ] Detect incompatible types
 *
 * [ ] Add warning and information diagnostics
 * [ ] Improve diagnostic messages
 * [ ] Add diagnostic severity levels
 * [ ] Separate lexer, parser and semantic analysis
 * [ ] Improve Emacs integration
 * [ ] Implement source highlighting with Emacs overlays
 *
 * [ ] Test against malformed C source files
 * [ ] Test large source files
 * [ ] Test deeply nested expressions
 * [ ] Test edge cases and escaped characters
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 0 here is to terminate any looping through this
static const char *keywords[] = {"if",  "else", "while", "for",    "return",
                                 "int", "char", "void",  "struct", 0};

// memcmp checks byte equivalence, 0 is equal
// n is string length
static int is_keyword(const char *source, size_t n) {
  for (int i = 0; keywords[i]; i++) {
    if (strlen(keywords[i]) == n && memcmp(keywords[i], source, n) == 0)
      return 1;
  }
  return 0;
}

static void highlight(size_t start_offset, size_t end_offset,
                      const char *comment) {
  printf("H %zu %zu %s\n", start_offset, end_offset, comment);
}

static void error(int line, int column, const char *message) {
  printf("E %d %d %s\n", line, column, message);
}

int main(void) {
  // size starts at 16b(64KiB)
  size_t capacity = 1 << 16, done_len = 0, r;
  char *source = malloc(capacity);

  if (!source)
    return 1;

  // if buffer almost full, double
  // TODO: check if necessary, could incrementally increase size so not
  // exponentialyes

  while (1) {
    if (done_len + 4096 >= capacity) {
      capacity *= 2;
      char *tmp = realloc(source, capacity);
      if (!tmp) {
        free(source);
        return 1;
      }
      source = tmp;
    }
    // 1 is size of each value in bytes -1 leaves space for null terminator
    r = fread(source + done_len, 1, capacity - done_len - 1, stdin);
    if (r == 0) // done reading
      break;
    done_len += r;
  }
  source[done_len] = 0; // null terminate

  struct {
    char character; // the bracket type
    int line,       // line opened
        column;     // column opened
  } stk[256];
  // stack of remembered brackets, if you need more than 256 you have aproblem

  int stack = 0;
  int line = 1;
  size_t i = 0;
  size_t line_start = 0;
  size_t last_progress = 0;
// gets the column of a piece of text (starts on one)
#define COL(p) ((int)((p) - line_start) + 1)

  // looks at char of i, decides on token, advances until token consumed, prints
  // output
  while (i < done_len) {
    if (i - last_progress >= 65536) {
      printf("P %zu\n", i);
      fflush(stdout);
      last_progress = i;
    }
    char c = source[i];
    size_t start = i;

    // counts newline
    if (c == '\n') {
      line++;
      i++;
      line_start = i;
      continue;
    }

    // whitespace
    if (isspace((unsigned char)c)) {
      i++;
      continue;
    }

    // comment, does not do comments like: if(foo != bar // comment)
    if (c == '/' && i + 1 < done_len && source[i + 1] == '/') {
      while (i < done_len && source[i] != '\n')
        i++;
      highlight(start, i, "comment");
      continue;
    }

    // block comment
    if (c == '/' && i + 1 < done_len && source[i + 1] == '*') {
      // remember where it started incase this cant find the end
      int line0 = line;
      int col0 = COL(i);
      int closed = 0;

      i += 2; // skip opening /*

      while (i < done_len) {
        if (source[i] == '*' && i + 1 < done_len && source[i + 1] == '/') {
          i += 2;
          closed = 1;
          break;
        }

        if (source[i] == '\n') {
          line++;
          line_start = i + 1;
        }

        i++;
      }

      if (!closed)
        error(line0, col0, "unterminated comment");

      highlight(start, i, "comment");
      continue;
    }

    // strigns
    if (c == '"') {
      i++; // skip opening "
      while (i < done_len && source[i] != '"' && source[i] != '\n') {
        if (source[i] == '\\' && i + 1 < done_len && source[i + 1] != '\n')
          i++;
        i++;
      }

      if (i < done_len && source[i] == '"')
        i++; // ignore trailing '
      else
        error(line, COL(start), "unterminated string ");

      highlight(start, i, "string");
      continue;
    }

    // number

    if (isdigit((unsigned char)c)) {
      while (i < done_len &&
             (isalnum((unsigned char)source[i]) || source[i] == '.'))
        i++;

      highlight(start, i, "constant");
      continue;
    }

    if (isalpha((unsigned char)c) || c == '_') {
      while (i < done_len &&
             (isalnum((unsigned char)source[i]) || source[i] == '_'))
        i++;

      if (is_keyword(source + start, i - start))
        highlight(start, i, "keyword");

      continue;
    }

    // puh brackets onto the stack
    if (c == '(' || c == '[' || c == '{') {
      if (stack < 256) {
        stk[stack].character = c;
        stk[stack].line = line;
        stk[stack].column = COL(i);
        stack++;
      }
    }

    // closing brackets, must match last opener
    else if (c == ')' || c == ']' || c == '}') {
      char want = (c == ')') ? '(' : (c == ']') ? '[' : '{';

      if (stack && stk[stack - 1].character == want)
        stack--; // match? remove it
      else
        error(line, COL(i), "mismatched closing parenthesis");
    }

    i++;
  }
  printf("P %zu\n", done_len);
  fflush(stdout);

  while (stack > 0) {
    stack--;
    error(stk[stack].line, stk[stack].column, "unclosed parenthises");
  }

  free(source);

  return 0;
}
