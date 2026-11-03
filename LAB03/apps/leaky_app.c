#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_RECORDS 100
#define MSG_SIZE 256

typedef struct {
    int id;
    char *name;
    int *scores;
} Record;

Record *create_record(int id, const char *name, int n)
{
    Record *r = malloc(sizeof(Record));

    r->id = id;
    r->name = malloc(strlen(name) + 1);
    strcpy(r->name, name);

    r->scores = malloc(sizeof(int) * n);

    for (int i = 0; i < n; i++)
        r->scores[i] = id * 10 + i;

    return r;
}

void delete_record(Record *r)
{
    /* Bug 1: Quen giai phong bo nho */
    /* free(r->name); */
    /* free(r->scores); */
    free(r);
}

void process_messages(int count)
{
    for (int i = 0; i < count; i++) {
        char *buf = malloc(MSG_SIZE);

        snprintf(buf, MSG_SIZE, "Message %d", i);

        if (i % 10 == 0)
            printf("%s\n", buf);

        /* Bug 2: Quen free(buf) */
        /* free(buf); */
    }
}

int main(void)
{
    Record **records = malloc(sizeof(Record *) * NUM_RECORDS);

    for (int i = 0; i < NUM_RECORDS; i++)
        records[i] = create_record(i, "EmbeddedLinuxStudent", 5);

    for (int i = 0; i < NUM_RECORDS; i++)
        delete_record(records[i]);

    free(records);

    process_messages(50);

    printf("Program finished\n");
    return 0;
}
