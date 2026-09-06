/*
 * harness.c
 * ---------
 * A tiny, beginner-friendly "LLM agent harness" written in plain C.
 *
 * This program simulates the basic shape of a real LLM agent harness:
 *   - a main loop that reads user input
 *   - a place to decide "does this need a tool, or the model?"
 *   - a mock model that stands in for a real LLM
 *   - a small conversation history (context window) that remembers
 *     the last 5 turns
 *   - a single tool (a calculator) that the harness can call directly
 *     instead of asking the "model" to do math
 *
 * Compile with:
 *   gcc -Wall -Wextra -std=c11 harness.c -o harness
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------- Constants ---------- */

#define INPUT_SIZE 256   /* holds up to 255 chars + null terminator */
#define MAX_TURNS 5      /* the harness only remembers the last 5 turns */

/*
 * A "Turn" represents one exchange in the conversation:
 * one user message and one assistant response.
 *
 * We store these as heap-allocated (malloc'd) strings, NOT as pointers
 * into the temporary input buffer used by fgets(). The input buffer is
 * reused every loop iteration, so if we stored a pointer to it directly,
 * every history entry would end up pointing at whatever the *most recent*
 * input was (or garbage). Dynamic memory gives each turn its own private,
 * permanent copy of the text.
 */
typedef struct {
    char *user;
    char *assistant;
} Turn;

/* The fixed-size "context window": an array of up to MAX_TURNS turns. */
static Turn history[MAX_TURNS];
static int turn_count = 0; /* how many turns are currently stored (0..MAX_TURNS) */

/* ---------- Function prototypes ---------- */

static char *dup_string(const char *src);
static void strip_newline(char *str);
static void flush_stdin_line(void);

static void add_turn(const char *user, const char *assistant);
static void free_history(void);
static void print_history(void);

static char *mock_model(const char *input);

static int is_tool_request(const char *input);
static char *process_tool(const char *input);
static int calculator(double a, char op, double b, double *result);

static void shutdown_harness(void);

/* ---------- Small string helpers ---------- */

/*
 * dup_string: allocates a new heap buffer exactly big enough for a copy
 * of src (including the null terminator) and copies it in.
 *
 * We can't rely on the POSIX-only strdup() here since the spec asks for
 * standard C only, so we implement the same idea by hand.
 *
 * If malloc() fails, we print an error and shut down cleanly rather than
 * returning NULL and risking a crash later when someone dereferences it.
 */
static char *dup_string(const char *src) {
    size_t len = strlen(src) + 1; /* +1 for the null terminator */
    char *copy = malloc(len);

    if (copy == NULL) {
        fprintf(stderr, "Fatal error: memory allocation failed.\n");
        free_history();
        exit(EXIT_FAILURE);
    }

    /* len already accounts for the null terminator, so a plain copy is safe */
    memcpy(copy, src, len);
    return copy;
}

/*
 * strip_newline: fgets() keeps the trailing '\n' if the whole line fit
 * in the buffer. This removes it so we can safely compare the string
 * against things like "exit" or "history" with strcmp().
 */
static void strip_newline(char *str) {
    size_t len = strlen(str);
    if (len > 0 && str[len - 1] == '\n') {
        str[len - 1] = '\0';
    }
}

/*
 * flush_stdin_line: if the user typed a line longer than our buffer can
 * hold, fgets() will NOT have read a trailing '\n' yet - the rest of the
 * line is still sitting in stdin. If we don't clear it out, the leftover
 * characters would be read as the *next* line of input, which would be
 * confusing. This reads and discards characters until it finds '\n' or EOF.
 */
static void flush_stdin_line(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {
        /* discard */
    }
}

/* ---------- Conversation history / context management ---------- */

/*
 * add_turn: stores one (user, assistant) pair in the history array.
 *
 * How the 5-turn history works:
 *   - While there is room (turn_count < MAX_TURNS), we just append the
 *     new turn at index turn_count and increase turn_count.
 *   - Once the array is full (a 6th turn arrives), we:
 *       1. free() the oldest turn's strings (index 0), since that memory
 *          is about to be discarded and would otherwise leak.
 *       2. shift every remaining turn one slot to the left, so index 0
 *          becomes the "new oldest" turn.
 *       3. write the newest turn into the now-empty last slot.
 *     This keeps the array always holding the most recent 5 turns, in
 *     chronological order from index 0 (oldest) to the last used index
 *     (newest).
 *
 * Both the user and assistant strings are duplicated with dup_string()
 * here, so the caller is free to reuse or free their own copies.
 */
static void add_turn(const char *user, const char *assistant) {
    char *user_copy = dup_string(user);
    char *assistant_copy = dup_string(assistant);

    if (turn_count < MAX_TURNS) {
        history[turn_count].user = user_copy;
        history[turn_count].assistant = assistant_copy;
        turn_count++;
        return;
    }

    /* History is full: evict the oldest turn (index 0) */
    free(history[0].user);
    free(history[0].assistant);

    /* Shift turns 1..MAX_TURNS-1 left by one slot */
    for (int i = 1; i < MAX_TURNS; i++) {
        history[i - 1] = history[i];
    }

    /* Place the new turn in the last slot */
    history[MAX_TURNS - 1].user = user_copy;
    history[MAX_TURNS - 1].assistant = assistant_copy;
}

/*
 * free_history: releases every string currently stored in history.
 * Called both on normal shutdown and on a fatal allocation failure.
 * Resets turn_count to 0 so the array is left in a clean, reusable state
 * (and so we never accidentally free the same pointer twice).
 */
static void free_history(void) {
    for (int i = 0; i < turn_count; i++) {
        free(history[i].user);
        free(history[i].assistant);
        history[i].user = NULL;
        history[i].assistant = NULL;
    }
    turn_count = 0;
}

/*
 * print_history: prints all stored turns in chronological order,
 * matching the format shown in the spec.
 */
static void print_history(void) {
    printf("Conversation History:\n");
    for (int i = 0; i < turn_count; i++) {
        printf("Turn %d\n", i + 1);
        printf("User: %s\n", history[i].user);
        printf("Assistant: %s\n", history[i].assistant);
    }
}

/* ---------- Mock model ---------- */

/*
 * mock_model: stands in for a real LLM call. It looks at the input text
 * and returns a canned, deterministic response depending on simple
 * keyword checks. In a real harness this function would instead make a
 * network call to an LLM API - here it's just string logic so the rest
 * of the harness has something to talk to.
 *
 * The returned string is heap-allocated; the caller is responsible for
 * freeing it once it's done with it (after storing a copy in history).
 */
static char *mock_model(const char *input) {
    if (strstr(input, "hello") != NULL) {
        return dup_string("Hello! How can I help you?");
    }

    if (strstr(input, "history") != NULL) {
        return dup_string("Conversation history is managed by the harness, not the model.");
    }

    /* Default: echo the input back inside a fixed-size buffer, then
     * duplicate it onto the heap so all mock_model() return paths are
     * consistent (always a fresh malloc'd string). */
    char buffer[INPUT_SIZE + 32];
    snprintf(buffer, sizeof(buffer), "Mock model received: %s", input);
    return dup_string(buffer);
}

/* ---------- Tool: calculator ---------- */

/*
 * calculator: performs one arithmetic operation.
 * Returns 0 on success, -1 specifically for division by zero.
 * This function has NO idea it's being called from a "tool" context -
 * it's just plain arithmetic, kept separate from both the model and the
 * parsing logic, so it's easy to test on its own.
 */
static int calculator(double a, char op, double b, double *result) {
    switch (op) {
        case '+':
            *result = a + b;
            return 0;
        case '-':
            *result = a - b;
            return 0;
        case '*':
            *result = a * b;
            return 0;
        case '/':
            if (b == 0.0) {
                return -1; /* division by zero */
            }
            *result = a / b;
            return 0;
        default:
            return -2; /* unsupported operator (shouldn't normally happen) */
    }
}

/*
 * is_tool_request: decides whether the user's input should be routed to
 * a tool instead of the mock model. This is the harness's "router" -
 * a real agent harness would look at model output (e.g. a tool_use
 * request) to decide this; here we keep it simple and just look at the
 * raw user text for a "calc" prefix.
 *
 * We require the text to start with "calc" followed by either the end
 * of the string or a space, so words like "calculate" don't accidentally
 * trigger the tool.
 */
static int is_tool_request(const char *input) {
    const char *prefix = "calc";
    size_t prefix_len = strlen(prefix);

    if (strncmp(input, prefix, prefix_len) != 0) {
        return 0;
    }

    char next = input[prefix_len];
    return (next == '\0' || next == ' ');
}

/*
 * process_tool: parses a "calc <number> <operator> <number>" command,
 * calls calculator(), and formats a response string - all WITHOUT
 * touching the mock model. Keeping tool logic and model logic in
 * separate functions is what lets the harness treat "call a tool" and
 * "ask the model" as two clearly distinct paths.
 *
 * Returns a heap-allocated string; caller must free it.
 */
static char *process_tool(const char *input) {
    double a = 0.0, b = 0.0, result = 0.0;
    char op = '\0';

    /* sscanf's "%lf %c %lf" skips leading whitespace before %lf
     * automatically, and the literal space before %c tells sscanf to
     * skip any whitespace before reading the operator character too. */
    int parsed = sscanf(input, "calc %lf %c %lf", &a, &op, &b);

    if (parsed != 3) {
        return dup_string("Invalid calculator command.");
    }

    if (op != '+' && op != '-' && op != '*' && op != '/') {
        return dup_string("Invalid calculator command.");
    }

    int status = calculator(a, op, b, &result);

    if (status == -1) {
        return dup_string("Error: division by zero.");
    }
    if (status == -2) {
        return dup_string("Invalid calculator command.");
    }

    char buffer[64];
    snprintf(buffer, sizeof(buffer), "Tool result: %.2f", result);
    return dup_string(buffer);
}

/* ---------- Shutdown ---------- */

/*
 * shutdown_harness: the single place that performs cleanup, so both the
 * "exit" command path and the EOF (Ctrl+D) path free memory in exactly
 * the same way. This avoids duplicating cleanup logic and avoids
 * accidentally freeing things twice.
 */
static void shutdown_harness(void) {
    free_history();
    printf("Shutting down harness.\n");
    printf("Memory cleaned successfully.\n");
}

/* ---------- Main program ---------- */

int main(void) {
    char input[INPUT_SIZE];

    printf("Mini LLM Harness Started\n");
    printf("Type \"exit\" to quit.\n");
    printf("Commands: calc <number> <operator> <number>, history\n");

    for (;;) {
        printf("User: ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            /* EOF (e.g. Ctrl+D) or a read error: shut down the same
             * way "exit" would. */
            printf("\n");
            shutdown_harness();
            return 0;
        }

        /* If the line didn't end in a newline, the input was longer than
         * our buffer - drain the rest of that line from stdin so it
         * doesn't get treated as the next command. */
        if (strchr(input, '\n') == NULL && !feof(stdin)) {
            flush_stdin_line();
        }

        strip_newline(input);

        if (strcmp(input, "exit") == 0) {
            shutdown_harness();
            return 0;
        }

        if (strcmp(input, "history") == 0) {
            /* Per spec: the "history" command itself is not stored as a
             * conversation turn - it's just a request to view state. */
            print_history();
            continue;
        }

        /* Empty input (user just pressed Enter) is handled safely: it's
         * just passed along as an ordinary (empty) message, no crash. */

        char *response = NULL;

        if (is_tool_request(input)) {
            response = process_tool(input);
        } else {
            response = mock_model(input);
        }

        printf("Assistant: %s\n", response);

        add_turn(input, response);

        /* add_turn() made its own copy of response, so we free our
         * temporary copy here to avoid leaking it. */
        free(response);
    }
}
