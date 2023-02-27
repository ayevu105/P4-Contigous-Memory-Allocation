/* @file memory.c
 * @brief The following pogram fufills the requirements of P4: Contigous Memory Allocation
 * and manages allocations within a memory pool of size MEMSIZE 80. 
 * @author Anthony Vu
 * @date 02/26/2023
 */
#include <stdio.h>
#include <stdlib.h>
#include <limits.h> 
#include <sys/types.h>
#include <string.h>
#include <stdbool.h>

#define MEMSIZE 80 // Max memory pool

typedef struct Process {
    char name;
    int begin;
    int end;
} process;

typedef struct Hole {
    int begin;
    int end;
} hole;

char memory[MEMSIZE + 1]; 
process processes[MEMSIZE]; 
hole holes[MEMSIZE]; 

int numHoles = 1; 
int allocations = 0; 
int maxHole = MEMSIZE; 

bool exitStatus = false; 

void executeCommand(char **command);

//makeUppercase converts alphabetical characters in a line to uppercase
void makeUppercase(char *line) {
  int i = 0;
  while (line[i] != 0) {
    if (i == 0 && (line[i] == 'r' || line[i] == 'R')) {
      if (line[i] == 'r') line[i] = 'R';
      break;
    }
    if (line[i] >= 'a' && line[i] <= 'z') {
      line[i] = line[i] - 32;
    }
    i++;
  }
}

//readline reads the command line inputted by the user
int  readline(char** buffer) {
  size_t len;   
  int size = getline(buffer, &len, stdin);
   if (size > 0) {
    (*buffer)[size - 1] = '\0';
  }
  return size;
}

//initializeMemory initializes the memory string with "." to be full
void initializeMemory() {
    for (int i = 0; i < MEMSIZE; i++) {
        memory[i] = '.';
    }
    memory[MEMSIZE] = '\0';
    holes[0].begin = 0;
    holes[0].end = MEMSIZE - 1;
}

//tokenize separates the line into tokens based on spaces
void tokenize(char *line, char **command) {
    char *token, *whitespace = " ";
    int index = 0;
    token = strtok(line, whitespace);
    while (token != NULL) {
        command[index] = token;
        index++;
        token = strtok(NULL, whitespace);
    }
    command[index] = NULL;
}

//max returns the max int between two numbers
int max(int n1, int n2) {
    return n1 > n2 ? n1 : n2;
}

//updateHoles the holes list and maxHole
void updateHoles() {
    int prev = 0; 
    numHoles = 0, maxHole = INT_MIN;

    for (int i = 0; i < allocations; i++) {
        process p = processes[i];
        if (p.begin > prev) {
            hole h = {prev, p.begin - 1};
            holes[numHoles] = h; 
            numHoles++;
            maxHole = max(h.end - h.begin + 1, maxHole);
        }
        prev = p.end + 1;
    }
    if (prev <= MEMSIZE - 1) {
        hole h = {prev, MEMSIZE - 1}; 
        holes[numHoles] = h;
        numHoles++;
        maxHole = max(h.end - h.begin + 1, maxHole);
    }
}

//allocate allocates the process with name and with the begin memory address
void allocate(char name, int begin, int size) {
    int wait = 1, i = 0;
    int end = begin + size - 1;

    for (int i = begin; i <= end; i++) {
        memory[i] = name;
    }
    process p = {name, begin, end};

    for (i = 0; i < allocations; i++) {
        if (p.begin < processes[i].begin) {
            for (int j = allocations; j > i; j--) {
                processes[j] = processes[j - 1];
            }
            processes[i] = p;
            allocations++;
            wait = 0;
            break;
        }
    }
   
    if (wait) {
        processes[i] = p;
        allocations++;
    }
    updateHoles();
}

//bigEnough returns a 1 if the size can fit into the hole 
int bigEnough(hole h, int size) {
    return (h.end - h.begin + 1) >= size;
}

//request allocates memory to process with name using a command algorithm
void request(char name, int size, char algo) {
    if (size > maxHole) {
        printf("Not enough memory\n");
        return;
    }
    if (algo == 'F') { //First-Fit
        for (int i = 0; i < numHoles; i++) {
            hole h = holes[i];
            if (bigEnough(h, size)) {
                allocate(name, h.begin, size);
                break;
            }
        }
    }
    else if (algo == 'B') { //Best-Fit
        int m = INT_MAX; 
        int minIndex = -1; 
        for (int i = 0; i < numHoles; i++) {
            hole h = holes[i];
            if (bigEnough(h, size) && h.end - h.begin + 1 < m) {
                m = h.end - h.begin + 1;
                minIndex = i;
            }
        }
        if (minIndex > -1) { 
            allocate(name, holes[minIndex].begin, size);
        }
    }
    else if (algo == 'W') { //WorstFit
        for (int i = 0; i < numHoles; i++) {
            hole h = holes[i];
            if (bigEnough(h, size) && h.end - h.begin + 1 == maxHole) {
                allocate(name, holes[i].begin, size);
                break;
            }
        }
    } else {
        printf("Unknown algorithm\n");
    }
}

//release frees all the allocations owned by a process
void release(char name) {
    for (int i = 0; i < allocations; i++) {
        process p = processes[i];
        if (p.name == name) {
            for (int i = p.begin; i <= p.end; i++) {
                memory[i] = '.'; 
            }
            for (int j = i; j < allocations; j++) {
                processes[j] = processes[j + 1];
            }
            i--;
            allocations--;
            updateHoles();
        }
    }
}

//compact slides all allocations to lower addresses which allows for free space to the right 
//as a contiguous block 
void compact() {
    int prev = 0;
    for (int i = 0; i < allocations; i++) {
        int size = processes[i].end - processes[i].begin;
        processes[i].begin = prev;
        processes[i].end = processes[i].begin + size;
        prev = processes[i].end + 1;
    }
    initializeMemory();
    for (int i = 0; i < allocations; i++) {
        int begin = processes[i].begin, end = processes[i].end;
        for (int j = begin; j <= end; j++) {
            memory[j] = processes[i].name;
        }
    }
    updateHoles();
}

//read opens a file and executes the command from each line
void read(char **command) {
    char line[MEMSIZE];
    FILE *file = fopen(command[1], "r"); 
    if (file == NULL) { 
        printf("Could not open file\n");
        return;
    }
    while (fgets(line, sizeof(line), file)) {
        printf("%s", line);
        makeUppercase(line);
        tokenize(line, command);
        if (*command[0] == 'E') { //Exit
            exitStatus = true;
            fclose(file);
        } else {
          executeCommand(command);
       }
    }
}

//executeCommmand executes the command in the command buffer
void executeCommand(char **command) {
    if (command[0] == NULL) {
        return;
    }
    if (*command[0] == 'A' && command[1] != NULL && command[2] != NULL 
        && command[3] != NULL && atoi(command[2]) > 0) {
        request(*command[1], atoi(command[2]), *command[3]);
    }
    else if (*command[0] == 'F' && command[1] != NULL) { //Free
        release(*command[1]);
    }
    else if (*command[0] == 'S') { //Show
        printf("%s", memory);
        printf("\n");
    }
    else if (*command[0] == 'R') { //Read 
        read(command);
    }
    else if (*command[0] == 'C') { //Compact 
        compact();
    }
    else if (*command[0] == 'E') { //Exit
        exitStatus = true;
    }
    else { // Command not found
        printf("Invalid command\n");
    }
}

int main() {
    char *command[MEMSIZE];
    char *cmdline = (char *) malloc(MEMSIZE * sizeof(char));
    int size;

    initializeMemory();
    while (1) {
        printf("command>");
        fflush(stdout);
        size = readline(&cmdline); 
        if (size > 0) {
            cmdline[size - 1] = 0;
            makeUppercase(cmdline);
            printf("%s\n", cmdline);
            tokenize(cmdline, command);
        }
        executeCommand(command);
        if (exitStatus) {
            break;
        }
    }
}
