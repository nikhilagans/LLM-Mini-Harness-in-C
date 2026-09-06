#!/bin/bash
#
# test_harness.sh
# ----------------
# A simple, beginner-friendly test script for harness.c.
#
# What it does:
#   1. Compiles harness.c with the exact required flags.
#   2. Feeds the compiled program various sequences of input (as if a user
#      typed them), by piping text into it with printf.
#   3. Checks the program's output for the text we expect to see.
#   4. Prints PASS/FAIL for each check and a summary at the end.
#
# Notes for understanding the output:
#   - When input is piped in (like we do here) instead of typed at a real
#     keyboard, the terminal does NOT echo the typed text back. So you will
#     NOT see "User: hello" in the output the way you do when running the
#     program interactively — you'll just see "User: " followed directly
#     by "Assistant: ...". The exception is the "history" command, which
#     explicitly prints the stored "User: <text>" lines itself.
#   - Exit code 0 from the harness after a test means it shut down cleanly.
#
# Usage:
#   chmod +x test_harness.sh
#   ./test_harness.sh
#
# Run this from the same directory as harness.c.

SOURCE_FILE="harness.c"
BINARY="./harness_test_build"

PASS_COUNT=0
FAIL_COUNT=0

# ---------- Step 1: Compile ----------

echo "=== Compiling $SOURCE_FILE ==="

# Capture compiler output so we can check for warnings, not just errors.
COMPILE_OUTPUT=$(gcc -Wall -Wextra -std=c11 "$SOURCE_FILE" -o "$BINARY" 2>&1)
COMPILE_STATUS=$?

if [ $COMPILE_STATUS -ne 0 ]; then
    echo "FAIL: Compilation failed. Fix compiler errors before running tests."
    echo "$COMPILE_OUTPUT"
    exit 1
fi

if [ -n "$COMPILE_OUTPUT" ]; then
    echo "WARNING: Compiler produced warnings:"
    echo "$COMPILE_OUTPUT"
    echo "(Tests will still run, but a clean submission should have zero warnings.)"
else
    echo "Compiled cleanly with no warnings."
fi
echo ""

# ---------- Helper: run a simple "does output contain this text" test ----------
#
# Arguments:
#   $1 = test name
#   $2 = input to feed the program (use \n for newlines, passed to printf)
#   $3 = text we expect to find somewhere in the output
run_test() {
    local name="$1"
    local input="$2"
    local expected="$3"

    local actual
    actual=$(printf "%b" "$input" | "$BINARY")

    if echo "$actual" | grep -qF "$expected"; then
        echo "PASS: $name"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        echo "FAIL: $name"
        echo "  Expected to find: $expected"
        echo "  --- Actual output ---"
        echo "$actual" | sed 's/^/  /'
        echo "  ---------------------"
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi
}

# ---------- Helper: run a test that also checks the exit code ----------
#
# Arguments:
#   $1 = test name
#   $2 = input to feed the program
#   $3 = expected exit code (usually 0)
run_test_exit_code() {
    local name="$1"
    local input="$2"
    local expected_code="$3"

    printf "%b" "$input" | "$BINARY" > /dev/null 2>&1
    local actual_code=$?

    if [ "$actual_code" -eq "$expected_code" ]; then
        echo "PASS: $name (exit code $actual_code)"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        echo "FAIL: $name (expected exit code $expected_code, got $actual_code)"
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi
}

echo "=== Running behavior tests ==="

# --- Mock model behavior ---
run_test "Greeting keyword triggers hello response" \
    "hello\nexit\n" \
    "Assistant: Hello! How can I help you?"

run_test "Unrecognized input falls back to echo response" \
    "what is a CPU\nexit\n" \
    "Assistant: Mock model received: what is a CPU"

run_test "History keyword inside a sentence is explained by the model" \
    "tell me about history\nexit\n" \
    "Conversation history is managed by the harness"

# --- Calculator tool behavior ---
run_test "Calculator addition" \
    "calc 5 + 3\nexit\n" \
    "Assistant: Tool result: 8.00"

run_test "Calculator subtraction" \
    "calc 10 - 4\nexit\n" \
    "Assistant: Tool result: 6.00"

run_test "Calculator multiplication" \
    "calc 6 * 7\nexit\n" \
    "Assistant: Tool result: 42.00"

run_test "Calculator division" \
    "calc 20 / 5\nexit\n" \
    "Assistant: Tool result: 4.00"

run_test "Calculator division by zero is caught" \
    "calc 5 / 0\nexit\n" \
    "Assistant: Error: division by zero."

run_test "Invalid calculator syntax is handled safely" \
    "calc hello\nexit\n" \
    "Assistant: Invalid calculator command."

run_test "Calculator tool is not confused with the model" \
    "calc 2 + 2\nexit\n" \
    "Assistant: Tool result: 4.00"

# --- Input handling ---
run_test "Empty input (just pressing Enter) does not crash" \
    "\nexit\n" \
    "Assistant: Mock model received:"

run_test_exit_code "Empty input followed by exit shuts down cleanly" \
    "\nexit\n" \
    0

# --- History command formatting ---
run_test "History command prints the Conversation History header" \
    "hello\nhistory\nexit\n" \
    "Conversation History:"

run_test "History command shows stored user text" \
    "hello\nhistory\nexit\n" \
    "User: hello"

run_test "History command shows stored assistant text" \
    "hello\nhistory\nexit\n" \
    "Assistant: Hello! How can I help you?"

echo ""
echo "=== Running structural tests (history limits, exit codes) ==="

# --- The "history" command itself should NOT be stored as a turn ---
# We call "history" three times in a row after a single real turn.
# If "history" were being stored as a turn, we would eventually see
# "Turn 2" appear. We expect to NEVER see "Turn 2" in this sequence,
# since only one real conversational turn ("hello") ever happened.
HISTORY_NOT_COUNTED_OUTPUT=$(printf "hello\nhistory\nhistory\nhistory\nexit\n" | "$BINARY")
if echo "$HISTORY_NOT_COUNTED_OUTPUT" | grep -q "^Turn 2$"; then
    echo "FAIL: The 'history' command is incorrectly being stored as a conversation turn"
    FAIL_COUNT=$((FAIL_COUNT + 1))
else
    echo "PASS: The 'history' command is not stored as a conversation turn"
    PASS_COUNT=$((PASS_COUNT + 1))
fi

# --- 5-turn sliding window / eviction test ---
# Send 6 distinct real turns, then ask for history.
# The oldest turn ("one") should have been evicted, and the newest
# 5 turns ("two" through "six") should remain, in order.
EVICTION_INPUT="one\ntwo\nthree\nfour\nfive\nsix\nhistory\nexit\n"
EVICTION_OUTPUT=$(printf "%b" "$EVICTION_INPUT" | "$BINARY")

if echo "$EVICTION_OUTPUT" | grep -q "User: one"; then
    echo "FAIL: Oldest turn ('one') was not evicted from history"
    FAIL_COUNT=$((FAIL_COUNT + 1))
else
    echo "PASS: Oldest turn ('one') was correctly evicted from history"
    PASS_COUNT=$((PASS_COUNT + 1))
fi

# Check that "two" is now the FIRST entry (Turn 1) after eviction.
FIRST_TURN_USER=$(echo "$EVICTION_OUTPUT" | awk '/^Turn 1$/{getline; print; exit}')
if [ "$FIRST_TURN_USER" = "User: two" ]; then
    echo "PASS: 'two' correctly shifted into the Turn 1 slot after eviction"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo "FAIL: Expected 'User: two' in the Turn 1 slot, got: $FIRST_TURN_USER"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# Check that exactly 5 turns are printed (Turn 1 through Turn 5, no Turn 6).
TURN_COUNT=$(echo "$EVICTION_OUTPUT" | grep -c "^Turn [0-9]*$")
if [ "$TURN_COUNT" -eq 5 ]; then
    echo "PASS: History correctly holds exactly 5 turns after eviction"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo "FAIL: Expected exactly 5 stored turns, found $TURN_COUNT"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# --- "exit" command test ---
run_test_exit_code "'exit' command shuts down with exit code 0" \
    "hello\nexit\n" \
    0

# --- EOF (Ctrl+D) handling test ---
# We never send "exit" here — the pipe just closes after "hello\n",
# simulating what happens when a user presses Ctrl+D. The harness should
# shut down cleanly, the same as if "exit" had been typed.
EOF_OUTPUT=$(printf "hello\n" | "$BINARY")
EOF_EXIT_CODE=$?

if echo "$EOF_OUTPUT" | grep -q "Shutting down harness." && [ "$EOF_EXIT_CODE" -eq 0 ]; then
    echo "PASS: EOF (Ctrl+D) triggers the same clean shutdown as 'exit'"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo "FAIL: EOF did not shut down cleanly (exit code $EOF_EXIT_CODE)"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# --- Overlong input line test ---
# Build a line of 500 'x' characters, which is longer than the 255-char
# input limit. The harness should not crash, and should still correctly
# read the next line ("hello") afterward instead of getting confused by
# leftover characters.
LONG_LINE=$(python3 -c "print('x' * 500)" 2>/dev/null || printf 'x%.0s' {1..500})
LONG_LINE_OUTPUT=$(printf "%s\nhello\nexit\n" "$LONG_LINE" | "$BINARY")
LONG_LINE_EXIT_CODE=$?

if [ "$LONG_LINE_EXIT_CODE" -eq 0 ] && echo "$LONG_LINE_OUTPUT" | grep -q "Assistant: Hello! How can I help you?"; then
    echo "PASS: Overlong input line does not crash the program or corrupt the next line"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo "FAIL: Overlong input line caused a crash or corrupted the next read"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# ---------- Optional: Valgrind memory check ----------

echo ""
echo "=== Optional: Valgrind memory check ==="

if command -v valgrind > /dev/null 2>&1; then
    VALGRIND_INPUT="hello\ncalc 5 + 3\ncalc 5 / 0\ncalc hello\none\ntwo\nthree\nfour\nfive\nsix\nhistory\nexit\n"
    VALGRIND_OUTPUT=$(printf "%b" "$VALGRIND_INPUT" | valgrind --leak-check=full --error-exitcode=99 "$BINARY" 2>&1)
    VALGRIND_STATUS=$?

    if [ "$VALGRIND_STATUS" -eq 99 ]; then
        echo "FAIL: Valgrind detected memory errors or leaks"
        echo "$VALGRIND_OUTPUT" | grep -A 5 "ERROR SUMMARY\|LEAK SUMMARY"
        FAIL_COUNT=$((FAIL_COUNT + 1))
    else
        echo "PASS: Valgrind reports no memory errors or leaks"
        PASS_COUNT=$((PASS_COUNT + 1))
    fi
else
    echo "Valgrind not installed — skipping memory check."
    echo "(Install it with: sudo apt install valgrind)"
fi

# ---------- Cleanup and summary ----------

rm -f "$BINARY"

echo ""
echo "=== Test Summary ==="
echo "Passed: $PASS_COUNT"
echo "Failed: $FAIL_COUNT"

if [ "$FAIL_COUNT" -eq 0 ]; then
    echo "All tests passed."
    exit 0
else
    echo "Some tests failed. See details above."
    exit 1
fi
