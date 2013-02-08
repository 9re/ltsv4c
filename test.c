#include "ltsv4c.h"
#include <stdio.h>
#include <string.h>

#define TEST(A) printf("%-72s-",#A);		   \
                if(A){puts(" OK");tests_passed++;} \
                else{puts(" FAIL");tests_failed++;}
#define STREQ(A, B) (A && B ? strcmp(A, B) == 0 : 0)

void test_suite_0(void);
void test_suite_1(void);

static int tests_passed;
static int tests_failed;

int main(void) {
    test_suite_0();
    test_suite_1();
    
    printf("Tests failed: %d\n", tests_failed);
    printf("Tests passed: %d\n", tests_passed);
  
    return 0;
}

void test_suite_0(void) {
    LTSV *ltsv = ltsv_parse_file("tests/test_0.txt");
    LTSV_Record *record;
    int ltsv_count;

    TEST(ltsv);
    if (!ltsv) {
        fprintf(stderr, "parse failed: tests/test_0.txt\n");
        return;
    }

    ltsv_count = ltsv_get_count(ltsv);
    TEST(ltsv_count == 4);
    if (ltsv_count != 4) {
        fprintf(stderr, "ltsv count doesn't match!\n");
	return;
    }
    record = ltsv_get_record(ltsv, 0);
    TEST(STREQ(ltsv_record_get_value(record, "a"), "05/02/2013:12:00:00 +0900"));
    TEST(STREQ(ltsv_record_get_value(record, "b"), "x.x.x.x"));
    TEST(STREQ(ltsv_record_get_value(record, "c"), "nn"));
    record = ltsv_get_record(ltsv, 1);
    TEST(STREQ(ltsv_record_get_value(record, "b"), "05/02/2013:12:01:00 +0900"));
    TEST(STREQ(ltsv_record_get_value(record, "c"), "x.y.x.y"));
    TEST(STREQ(ltsv_record_get_value(record, "a"), "nm"));
    record = ltsv_get_record(ltsv, 2);
    TEST(STREQ(ltsv_record_get_value(record, "c"), "05/02/2013:12:02:00 +0900"));
    TEST(STREQ(ltsv_record_get_value(record, "b"), "x.z.x.z"));
    TEST(STREQ(ltsv_record_get_value(record, "utf-8"), "あいうえお"));
    record = ltsv_get_record(ltsv, 3);
    TEST(STREQ(ltsv_record_get_value(record, "host"), "127.0.0.1"));
    TEST(STREQ(ltsv_record_get_value(record, "ident"), "-"));
    TEST(STREQ(ltsv_record_get_value(record, "time"), "[10/Oct/2000:13:55:36 -0700]"));
    TEST(STREQ(ltsv_record_get_value(record, "req"), "GET /apache_pb.gif HTTP/1.0"));
    TEST(STREQ(ltsv_record_get_value(record, "status"), "200"));
    TEST(STREQ(ltsv_record_get_value(record, "size"), "2326"));
    TEST(STREQ(ltsv_record_get_value(record, "referer"), "http://www.example.com/start.html"));
    TEST(STREQ(ltsv_record_get_value(record, "ua"), "Mozilla/4.08 [en] (Win98; I ;Nav)"));

    ltsv_free(ltsv);
    
    ltsv = ltsv_parse_string("label:text\thoge:fuga\nmm:value:1");
    ltsv_count = ltsv_get_count(ltsv);
    TEST(ltsv_count == 2);
    if (ltsv_count != 2) {
      fprintf(stderr, "ltsv count doesn't match!%d \n", ltsv_count);
	return;
    }
    record = ltsv_get_record(ltsv, 0);
    TEST(STREQ(ltsv_record_get_value(record, "label"), "text"));
    TEST(STREQ(ltsv_record_get_value(record, "hoge"), "fuga"));
    record = ltsv_get_record(ltsv, 1);
    TEST(STREQ(ltsv_record_get_value(record, "mm"), "value:1"));

    ltsv_free(ltsv); 
}

void test_suite_1(void) {
    TEST(ltsv_parse_string("k") == NULL);
    TEST(ltsv_parse_string("a:b\nk:v:\t~:v") == NULL);
    TEST(ltsv_parse_string(":b\nk:v:\t~:v") == NULL);
    TEST(ltsv_parse_string("a:b\ta:1") == NULL);
}
