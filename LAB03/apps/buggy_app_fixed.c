#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Bug 1: Array out of bounds */
void process_data(int *data, int len)
{
    int sum = 0;

    for (int i = 0; i < len; i++) {
        sum += data[i];
    }

    printf("Sum = %d\n", sum);
}

/* Bug 2: Use after free */
char *create_message(const char *text)
{
    char *buf = malloc(strlen(text) + 1);

    if (!buf)
        return NULL;

    strcpy(buf, text);

    return buf;
}

/* Bug 3: Integer overflow */
long long calculate(int a, int b)
{
    long long result = (long long)a * b;

    printf("Result = %lld\n", result);
    return result;
}

int main(void)
{
    int arr[5] = {10, 20, 30, 40, 50};

    setbuf(stdout, NULL);

    printf("=== LAB03 Debugging ===\n");

    process_data(arr, 5);

    char *msg = create_message("Embedded Linux");
    printf("Message: %s\n", msg);
    free(msg);

    calculate(100000, 100000);

    return 0;
}
