

# ECE 309 Project 1 - Vibe Coding Log

## AI Tool

Claude.ai

## 1. Initial Prompt

I started by giving Claude a detailed specification for the architecture and requirements of my LLM mini-harness. I decided that it was important to give a very 
detailed prompt as it would lead to less debugging and problems in the future. 

### Prompt

I need to build a beginner-friendly LLM mini-harness in standard C for an ECE 309 class project. I want you to act as the junior developer while I act as the system architect.
Please follow this specification exactly. Before writing any code, briefly restate the architecture and program flow so I can verify that you understood the requirements.

1. General Requirements
Language: C
Environment: POSIX/Linux environment using GCC
The program must compile with:
gcc -Wall -Wextra -std=c11 harness.c -o harness
Do not use any external libraries.
Use only standard C libraries such as:
stdio.h
stdlib.h
string.h
ctype.h if necessary
Keep the program beginner-friendly and avoid unnecessarily advanced C techniques.
Use clear function names and comments explaining the purpose of each major section.

2. Program Architecture
The program should behave like a very small LLM agent harness.
The harness should have these components:
Main interaction loop
User input handling
Mock LLM/model function
Conversation history/context management
Tool detection and execution
Safe shutdown and memory cleanup
The main program should coordinate these components rather than putting all functionality directly inside main().

3. Main Program State Machine
The program should follow this exact flow:
START
Initialize an empty conversation history.
Print a short message explaining that the mini-harness has started.
Enter a loop.
Prompt the user with:
User:
Read the user's input using fgets().
Safely remove the trailing newline if one exists.
If the user enters "exit":
stop the loop
free all dynamically allocated memory
print a shutdown message
terminate normally
Otherwise:
store the user's message in conversation history
determine whether the message requests a supported tool
if a tool is needed, execute the tool
otherwise send the input to the mock model function
print the resulting response using:
Assistant: <response>
Store the assistant response in conversation history.
Continue the loop.
END

4. Input Requirements
Use fgets() instead of scanf() for user input.
Support input strings up to 255 characters.
Prevent buffer overflows.
Handle empty input safely.
Do not crash if the user presses Enter without typing anything.
Remove the newline character left by fgets() before comparing strings.

5. Mock Model Function
Create a separate function that simulates an LLM.
Suggested function:
char *mock_model(const char *input);
The mock model should behave deterministically.
Rules:
If the input contains the word "hello", return a greeting such as:
"Hello! How can I help you?"
If the input contains the word "history", return a message indicating that conversation history is managed by the harness.
Otherwise return a response such as:
"Mock model received: <user input>"
The returned response must be stored safely in memory.
Do not call a real LLM API.

6. Conversation History / Context Management
The harness must remember only the most recent 5 conversation turns.
For this project, define one turn as:
one user message
one assistant response
Create a structure such as:
typedef struct {
char *user;
char *assistant;
} Turn;
Maintain an array capable of storing 5 Turn objects.
Requirements:
Dynamically allocate memory for stored user and assistant strings.
Never store direct pointers to the temporary input buffer.
When fewer than 5 turns exist, append the new turn.
When a 6th turn is added:
free the memory belonging to the oldest turn
shift the remaining turns toward the beginning of the array
store the newest turn at the end
Keep a variable that tracks the current number of stored turns.
All allocated strings must eventually be freed before program termination.
Create helper functions for context management instead of putting all memory operations inside main().
Suggested functions:
void add_turn(...);
void free_history(...);
void print_history(...);

7. History Command
Add a simple command:
history
If the user enters exactly:
history
the harness should print the currently stored conversation turns in chronological order.
Example:
Conversation History:
Turn 1
User: hello
Assistant: Hello! How can I help you?
Turn 2
User: testing
Assistant: Mock model received: testing
Do not treat the "history" command itself as a normal conversation turn unless necessary. Prefer not storing it.

8. Tool Execution
The harness must demonstrate how an LLM agent can call a tool for a task better suited for normal software.
Implement one calculator tool.
Use the following command syntax:
calc <number> <operator> <number>
Examples:
calc 5 + 3
calc 10 - 4
calc 6 * 7
calc 20 / 5
Supported operators:
Addition
Subtraction
Multiplication
Division
/
Create a separate function for tool execution.
Behavior:
Detect whether the user input begins with "calc ".
Parse the two numbers and operator.
Call the calculator function.
Return the result to the user as the assistant response.
Example:
User: calc 5 + 3
Assistant: Tool result: 8.00
Handle invalid calculator syntax safely.
Example:
User: calc hello
Assistant: Invalid calculator command.
Also detect division by zero.
Example:
User: calc 5 / 0
Assistant: Error: division by zero.
The calculator logic should not be implemented inside the mock model function. Tool execution and model behavior must remain separate.

9. Separation of Responsibilities
Organize the program so each function has a clear job.
At minimum, I would like functions similar to:
main()
mock_model()
add_turn()
free_history()
print_history()
calculator()
process_tool()
You may slightly modify these names or function signatures if needed, but keep the same architectural separation.
main() should primarily coordinate the program flow.
10. Memory Safety Requirements

Memory management is an important part of this project.
Requirements:
Use malloc() where appropriate for stored strings.
Check whether malloc() returned NULL.
Free memory when:
an old history entry is removed
the program exits
Never free the same pointer twice.
Never access memory after it has been freed.
Avoid memory leaks.
Do not use unsafe functions such as gets().
Prefer strncpy(), snprintf(), or carefully sized allocations where appropriate.
The program should be compatible with memory-checking tools such as Valgrind.
11. Error Handling

The program should handle:
empty input
overly long input without crashing
invalid calculator commands
division by zero
failed memory allocations
EOF from standard input, such as Ctrl+D
If EOF occurs, perform the same memory cleanup used for a normal exit.
12. Expected Example Run

The program should approximately behave like this:
Mini LLM Harness Started
Type "exit" to quit.
Commands: calc <number> <operator> <number>, history
User: hello
Assistant: Hello! How can I help you?
User: calc 12 * 4
Assistant: Tool result: 48.00
User: what is a CPU
Assistant: Mock model received: what is a CPU
User: history
Conversation History:
Turn 1
User: hello
Assistant: Hello! How can I help you?
Turn 2
User: calc 12 * 4
Assistant: Tool result: 48.00
Turn 3
User: what is a CPU
Assistant: Mock model received: what is a CPU
User: exit
Shutting down harness.
Memory cleaned successfully.
13. Code Style

Because I am a beginner in C:
Keep the implementation straightforward.
Do not use linked lists unless absolutely necessary.
Do not use function pointers.
Do not use threads.
Do not use networking.
Do not use external APIs.
Do not create unnecessary abstractions.
Add comments explaining:
why dynamic memory is needed
how the 5-turn history works
where memory is allocated
where memory is freed
how the harness distinguishes a tool request from a mock-model request
14. Deliverables

First generate only:
harness.c
After showing the code, explain:
The overall architecture.
How input travels through the harness.
How the mock model differs from the calculator tool.
How conversation history is stored.
How the oldest history entry is removed.
Where malloc() is used.
Where free() is used.
How I can manually test the program.
Do not generate the testing script yet. I will request that separately after I compile and test harness.c.

### Result

Claude generated `harness.c` based on my specifications.

The program included:
- Terminal user input
- Mock LLM responses
- Five-turn conversation history
- Calculator tool
- Dynamic memory management
- Error handling

---

## 2. Changes / Debugging

There were no additional code-generation prompts needed due to my very detailed initial SSD prompt. 

---

## 3. Compilation and Manual Testing

I compiled the program using:

```bash
gcc -Wall -Wextra -std=c11 harness.c -o harness
```

### Compilation Result

```text
nikhil_agans@NikhilAgans:~/ece309_harness$ gcc -Wall -Wextra -std=c11 harness.c -o harness
nikhil_agans@NikhilAgans:~/ece309_harness$ ls
harness  harness.c
```

The program compiled successfully with no warnings or errors and produced the `harness` executable.

### Manual Testing

I ran the program using:

```bash
./harness
```

I manually tested:

- `hello`
- Normal user input
- Calculator operations
- Division by zero
- `history`
- `exit`

### Manual Test Results

```text
nikhil_agans@NikhilAgans:~/ece309_harness$ ./harness
Mini LLM Harness Started
Type "exit" to quit.
Commands: calc <number> <operator> <number>, history
User: hello
Assistant: Hello! How can I help you?
User: random text
Assistant: Mock model received: random text
User: calc 5 + 3
Assistant: Tool result: 8.00
User: calc 10/0
Assistant: Error: division by zero.
User: history
Conversation History:
Turn 1
User: hello
Assistant: Hello! How can I help you?
Turn 2
User: random text
Assistant: Mock model received: random text
Turn 3
User: calc 5 + 3
Assistant: Tool result: 8.00
Turn 4
User: calc 10/0
Assistant: Error: division by zero.
User: hello
Assistant: Hello! How can I help you?
User: history
Conversation History:
Turn 1
User: hello
Assistant: Hello! How can I help you?
Turn 2
User: random text
Assistant: Mock model received: random text
Turn 3
User: calc 5 + 3
Assistant: Tool result: 8.00
Turn 4
User: calc 10/0
Assistant: Error: division by zero.
Turn 5
User: hello
Assistant: Hello! How can I help you?
User: history
Conversation History:
Turn 1
User: hello
Assistant: Hello! How can I help you?
Turn 2
User: random text
Assistant: Mock model received: random text
Turn 3
User: calc 5 + 3
Assistant: Tool result: 8.00
Turn 4
User: calc 10/0
Assistant: Error: division by zero.
Turn 5
User: hello
Assistant: Hello! How can I help you?
User: history
Conversation History:
Turn 1
User: hello
Assistant: Hello! How can I help you?
Turn 2
User: random text
Assistant: Mock model received: random text
Turn 3
User: calc 5 + 3
Assistant: Tool result: 8.00
Turn 4
User: calc 10/0
Assistant: Error: division by zero.
Turn 5
User: hello
Assistant: Hello! How can I help you?
User: exit
Shutting down harness.
Memory cleaned successfully.
nikhil_agans@NikhilAgans:~/ece309_harness$ 
```

The manual tests confirmed that the mock model, calculator tool, error handling, conversation history, and exit behavior worked as expected.
---

## 4. Automated Testing Prompt

After manually testing `harness.c`, I asked Claude to generate an automated Bash testing script.

### Prompt

I have a compiled C program named harness. Write a very simple Bash script (for 
Linux/Mac) that automatically sends the word 'hello', followed by the word 'exit', into the 
program to test if it works.

### Result

Claude generated `test.sh` to automatically test the program.

---

## 5. Automated Testing Results

I ran the test script using:

```bash
chmod +x test.sh
./test.sh
```

Final result:

```text
=== Compiling harness.c ===
Compiled cleanly with no warnings.

=== Running behavior tests ===
PASS: Greeting keyword triggers hello response
PASS: Unrecognized input falls back to echo response
PASS: History keyword inside a sentence is explained by the model
PASS: Calculator addition
PASS: Calculator subtraction
PASS: Calculator multiplication
PASS: Calculator division
PASS: Calculator division by zero is caught
PASS: Invalid calculator syntax is handled safely
PASS: Calculator tool is not confused with the model
PASS: Empty input (just pressing Enter) does not crash
PASS: Empty input followed by exit shuts down cleanly (exit code 0)
PASS: History command prints the Conversation History header
PASS: History command shows stored user text
PASS: History command shows stored assistant text

=== Running structural tests (history limits, exit codes) ===
PASS: The 'history' command is not stored as a conversation turn
PASS: Oldest turn ('one') was correctly evicted from history
PASS: 'two' correctly shifted into the Turn 1 slot after eviction
PASS: History correctly holds exactly 5 turns after eviction
PASS: 'exit' command shuts down with exit code 0 (exit code 0)
PASS: EOF (Ctrl+D) triggers the same clean shutdown as 'exit'
PASS: Overlong input line does not crash the program or corrupt the next line

=== Optional: Valgrind memory check ===
PASS: Valgrind reports no memory errors or leaks

=== Test Summary ===
Passed: 23
Failed: 0
All tests passed.
nikhil_agans@NikhilAgans:~/ece309_harness$ 

```

The tests verified the mock model, calculator, invalid input handling, conversation history, five-turn history limit, exit behavior, EOF handling, and long-input handling.

---

## 6. Final Result

The final project contains:

- `harness.c` - LLM mini-harness implementation
- `test.sh` - Automated testing script
- `vibe_coding_log.md` - Vibe coding and prompt documentation
- `README.md` - Project instructions and information

The program compiled successfully and passed all 23 automated tests.

## 7. Reflection

In this project I learned the importance and effiecency of giving AI a very detailed SSD prompt. Spending the time to create this prompt led to minimal 
errors in the compiling of the code.
