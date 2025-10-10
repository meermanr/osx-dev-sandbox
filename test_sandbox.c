#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main() {
    printf("=== Sandbox Test Program ===\n\n");

    // Test 1: Write to current directory (should succeed)
    printf("[TEST 1] Writing to current directory...\n");
    FILE *f1 = fopen("test_output.txt", "w");
    if (f1) {
        fprintf(f1, "Success: Write to working directory allowed\n");
        fclose(f1);
        printf("  ✓ SUCCESS: Created test_output.txt\n");
    } else {
        printf("  ✗ FAILED: Could not write to current directory\n");
    }

    // Test 2: Write to home directory (should fail)
    printf("\n[TEST 2] Writing to home directory...\n");
    char home_path[256];
    snprintf(home_path, sizeof(home_path), "%s/sandbox_test.txt", getenv("HOME"));
    FILE *f2 = fopen(home_path, "w");
    if (f2) {
        fprintf(f2, "This should not succeed\n");
        fclose(f2);
        printf("  ✗ UNEXPECTED: Write to home directory succeeded (should have been blocked)\n");
    } else {
        printf("  ✓ SUCCESS: Write to home directory blocked\n");
    }

    // Test 3: Write to /tmp (should succeed)
    printf("\n[TEST 3] Writing to /tmp...\n");
    FILE *f3 = fopen("/tmp/sandbox_test.txt", "w");
    if (f3) {
        fprintf(f3, "Success: Write to /tmp allowed\n");
        fclose(f3);
        printf("  ✓ SUCCESS: Created /tmp/sandbox_test.txt\n");
    } else {
        printf("  ✗ FAILED: Could not write to /tmp\n");
    }

    // Test 4: Read system files (should succeed)
    printf("\n[TEST 4] Reading system files...\n");
    FILE *f4 = fopen("/etc/hosts", "r");
    if (f4) {
        fclose(f4);
        printf("  ✓ SUCCESS: Can read /etc/hosts\n");
    } else {
        printf("  ✗ FAILED: Could not read /etc/hosts\n");
    }

    // Test 5: Spawn child process
    printf("\n[TEST 5] Spawning child process...\n");
    pid_t pid = fork();
    if (pid == 0) {
        // Child process - try to write
        FILE *f5 = fopen("child_output.txt", "w");
        if (f5) {
            fprintf(f5, "Child process write\n");
            fclose(f5);
            printf("  ✓ SUCCESS: Child process can write to working directory\n");
        } else {
            printf("  ✗ FAILED: Child process cannot write\n");
        }
        exit(0);
    } else if (pid > 0) {
        // Parent waits for child
        int status;
        waitpid(pid, &status, 0);
    }

    printf("\n=== Test Complete ===\n");
    printf("Check test_output.txt and child_output.txt in current directory\n");

    return 0;
}
