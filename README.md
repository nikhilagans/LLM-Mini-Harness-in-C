# LLM-Mini-Harness-in-C
 # ECE 309 LLM Mini-Harness

A minimal LLM agent harness written in C for ECE 309. The program simulates the basic structure of an LLM agent by accepting user input, generating mock model responses, maintaining conversation history, and executing a calculator tool.

## Features

- Terminal-based user interaction
- Deterministic mock LLM responses
- Five-turn conversation history
- Dynamic memory management
- Calculator tool
- Addition, subtraction, multiplication, and division
- Invalid calculator input handling
- Division-by-zero handling
- Automated Bash testing

## Requirements

- POSIX/Linux environment
- GCC
- Bash

The project was developed and tested using Ubuntu through WSL.

## Compilation

Compile the program with:

```bash
gcc -Wall -Wextra -std=c11 harness.c -o harness
```

## Running

Run the compiled program with:

```bash
./harness
```

The program will display:

```text
Mini LLM Harness Started
Type "exit" to quit.
Commands: calc <number> <operator> <number>, history
User:
```

## Commands

### Normal Input

Normal input is sent to the mock model.

Example:

```text
User: random text
Assistant: Mock model received: random text
```

### Hello

Input containing `hello` produces a greeting response.

### Calculator

Use:

```text
calc <number> <operator> <number>
```

Supported operators:

- `+` Addition
- `-` Subtraction
- `*` Multiplication
- `/` Division

Example:

```text
User: calc 5 + 3
Assistant: Tool result: 8.00
```

Division by zero is handled safely:

```text
User: calc 10 / 0
Assistant: Error: division by zero.
```

### History

Enter:

```text
history
```

to display the stored conversation history.

The harness stores the five most recent conversation turns. A turn consists of one user message and one assistant response.

### Exit

Enter:

```text
exit
```

to shut down the harness and clean up allocated memory.

## Automated Testing

The project includes an AI-generated Bash testing script named `test.sh`.

Make the script executable:

```bash
chmod +x test.sh
```

Run the tests:

```bash
./test.sh
```

Final testing result:

```text
Passed: 22
Failed: 0
All tests passed.
```

The automated tests verify model responses, calculator operations, invalid input handling, conversation history, the five-turn history limit, exit behavior, EOF handling, and overlong input handling.

## Project Files

- `harness.c` - Main LLM mini-harness implementation
- `test.sh` - Automated Bash testing script
- `vibe_coding_log.md` - Documentation of the AI-assisted development process
- `README.md` - Project documentation

## AI-Assisted Development

Claude.ai was used to assist with code generation and automated test generation. The prompts and development process are documented in `vibe_coding_log.md`.
