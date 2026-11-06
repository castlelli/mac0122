/* Includes the source file to reach its static functions; the comparator's
   own main is renamed so that it does not clash with the one below. */
#define main comparator_main
#include "comparator.c"
#undef main

#define MAX_TEXT_LEN 64

static int failure_count = 0;

static void check_removal(const char *text, const char *expected_text)
{
    char buffer[MAX_TEXT_LEN];

    strcpy(buffer, text);
    remove_carriage_returns(buffer, strlen(buffer));
    if (strcmp(buffer, expected_text) != 0) {
        printf("expected: \"%s\"\nobtained: \"%s\"\n\n", expected_text, buffer);
        failure_count++;
    }
}

int main(void)
{
    check_removal("", "");
    check_removal("\r", "");
    check_removal("no ending", "no ending");
    check_removal("one\ntwo\n", "one\ntwo\n");
    check_removal("one\r\ntwo\r\n", "one\ntwo\n");
    check_removal("<!DOCTYPE html>\r\n<html>", "<!DOCTYPE html>\n<html>");

    return failure_count == 0 ? 0 : 1;
}
