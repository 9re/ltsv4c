#include "ltsv4c.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ERROR                          0
#define SUCCESS                        1
#define STARTING_CAPACITY              16
#define is_tab(c)                      (c == 0x09)
#define is_newline(c)                  (c == 0x0d || c == 0x0a)
#define skip_char(str)                 ((*str)++)
#define MAX(a, b)                      ((a) > (b) ? (a) : (b))

typedef int(*LTSV_Char_Test_Func_Ptr)(const unsigned char);

struct ltsv_record_t {
    const char **labels;
    const char **values;
    size_t       count;
    size_t       capacity;
};

struct ltsv_t {
    LTSV_Record **records;
    size_t        count;
    size_t        capacity;
};

static int try_realloc(void **ptr, size_t new_size) {
    void *reallocated_ptr = realloc(*ptr, new_size);
    if (!reallocated_ptr) { return ERROR; }
    *ptr = reallocated_ptr;
    return SUCCESS;
}

static char * ltsv_strndup(const char *string, size_t n) {
    char *output_string = (char*)malloc(n + 1);
    if (!output_string) { return NULL; }
    output_string[n] = '\0';
    strncpy(output_string, string, n);
    return output_string;
}

/* LTSV_Record */
static LTSV_Record * ltsv_record_init(void) {
    LTSV_Record *new_obj = (LTSV_Record*)malloc(sizeof(LTSV_Record));
    if (!new_obj) { return NULL; }
    new_obj->labels = (const char**)NULL;
    new_obj->values = (const char**)NULL;
    new_obj->capacity = 0;
    new_obj->count = 0;
    return new_obj;
}

static int ltsv_record_resize(LTSV_Record *record, size_t capacity) {
    if (try_realloc((void**)&record->labels, capacity * sizeof(char*)) == ERROR) { return ERROR; }
    if (try_realloc((void**)&record->values, capacity * sizeof(char*)) == ERROR) { return ERROR; }
    record->capacity = capacity;
    return SUCCESS;
}

static void ltsv_record_free(LTSV_Record *record) {
    while(record->count--) {
        free((void*)record->labels[record->count]);
        free((void*)record->values[record->count]);
    }
    free((void*)record->labels);
    free((void*)record->values);
    free((void*)record);
}

static int ltsv_record_add_entry(LTSV_Record *record, const char *key, const char *value) {
    size_t index;
    if (record->count >= record->capacity) {
        size_t new_capacity = MAX(record->capacity * 2, STARTING_CAPACITY);
        if (ltsv_record_resize(record, new_capacity) == ERROR) { return ERROR; }
    }
    if (ltsv_record_get_value(record, key) != NULL) { return ERROR; }
    index = record->count;
    record->labels[index] = ltsv_strndup(key, strlen(key));
    if (!record->labels[index]) { return ERROR; }
    record->values[index] = value;
    record->count++;
    return SUCCESS;
}

static const char * ltsv_record_nget_value(const LTSV_Record *record, const char *key, size_t n) {
    size_t i, key_length;
    for (i = 0; i < ltsv_record_get_count(record); i++) {
        key_length = strlen(record->labels[i]);
        if (key_length != n) { continue; }
        if (strncmp(record->labels[i], key, n) == 0) { return record->values[i]; }
    }
    return NULL;
}

/* LTSV */
static LTSV * ltsv_init(void) {
    LTSV *new_ltsv = (LTSV*)malloc(sizeof(LTSV));
    if (!new_ltsv) { return NULL; }
    new_ltsv->records = (LTSV_Record**)NULL;
    new_ltsv->capacity = 0;
    new_ltsv->count = 0;
    return new_ltsv;
}

static int ltsv_resize(LTSV *ltsv, size_t capacity) {
    if (try_realloc((void**)&ltsv->records, capacity * sizeof(LTSV_Record*)) == ERROR) { return ERROR; }
    ltsv->capacity = capacity;
    return SUCCESS;
}

static int ltsv_add(LTSV *ltsv, LTSV_Record *record) {
    if (ltsv->count >= ltsv->capacity) {
        size_t new_capacity = MAX(ltsv->capacity * 2, STARTING_CAPACITY);
        if (!ltsv_resize(ltsv, new_capacity)) { return ERROR; }
    }
    ltsv->records[ltsv->count] = record;
    ltsv->count++;
    return SUCCESS;
}



/* %x01-08 / %x0B / %x0C / %x0E-FF */
static int field_char_test_func(const unsigned char c) {
    return (c > 0x0 && c < 0x09) || c == 0x0b || c == 0x0c || (c > 0x0e);
}

/* %x30-39 / %x41-5A / %x61-7A / "_" / "." / "-" ;; [0-9A-Za-z_.-] */
static int label_char_test_func(const unsigned char c) {
    return (c >= 0x30 && c <= 0x39) || (c >= 0x41 && c <= 0x5a) || (c >= 0x61 && c <= 0x7a) ||
           c == '_' || c == '.' || c == '-';
}

static const char *parse_string(const char **string, LTSV_Char_Test_Func_Ptr ptr_test_func) {
    const char *string_start = *string;
    char *output;

    while (ptr_test_func((unsigned char)**string)) {
        if (**string == '\0') {
            break;
        }
        skip_char(string);
    }
    output = ltsv_strndup(string_start, *string  - string_start);
    if (!output) { return NULL; }
    return output;
}

static LTSV_Record *parse_record(const char **string) {
    LTSV_Record *output_record = ltsv_record_init();
    const char *label, *value;
    if (!output_record) {
        fprintf(stderr, "couldn't allocate LTSV_Record!\n");
        return NULL;
    }
    while (**string != '\0') {
        label = parse_string(string, &label_char_test_func);
	if (!label) {
	    fprintf(stderr, "could not parse label!\n");
	    goto bail;
	}
	if (strlen(label) == 0) {
	    if (is_newline(**string)) {
	        return output_record;
	    }

	    fprintf(stderr, "invalid character for label: %c\n", **string);
	    
	    goto bail;
	}
	if (**string != ':') {
	    fprintf(stderr, "':' not found: '%c' found instead!\n", **string);

	    goto bail;
	}
	skip_char(string);
	value = parse_string(string, &field_char_test_func);

	if (ltsv_record_get_value(output_record, label)) {
	    fprintf(stderr, "dupliate entry for record: %s\n", label);
	    
	    goto bail;
	}
	
	ltsv_record_add_entry(output_record, label, value);

	if (**string == '\t') {
	    skip_char(string);
	}
    }
    
    return output_record;

 bail:
    ltsv_record_free(output_record);
    return NULL;
}

static LTSV *parse_ltsv(const char **string) {
    LTSV *ltsv = ltsv_init();
    LTSV_Record *record;
    if (!ltsv) {
        fprintf(stderr, "couldn't allocate LTSV!\n");
	return NULL;
    }
    while (**string != '\0') {
        record = parse_record(string);
	if (!record) {
	    if (!is_newline(**string)) {
	        goto bail;  
	    }

	    fprintf(stderr, "string: %c\n", **string);

	    return ltsv;
	}
	
	ltsv_add(ltsv, record);
	if (**string == 0x0d) {
	    skip_char(string);
	}
	if (**string == 0x0a) {
	    skip_char(string);
	}
    }
    
    return ltsv;
    
 bail:
    ltsv_free(ltsv);
    return NULL;
}


LTSV *ltsv_parse_file(const char *filename) {
    FILE *fp = fopen(filename, "r");
    size_t file_size;
    char *file_contents;
    LTSV *output_value;
    if (!fp) { return NULL; }
    fseek(fp, 0L, SEEK_END);
    file_size = ftell(fp);
    rewind(fp);
    file_contents = (char*)malloc(sizeof(char) * (file_size + 1));
    if (!file_contents) { fclose(fp); return NULL; }
    fread(file_contents, file_size, 1, fp);
    fclose(fp);
    file_contents[file_size] = '\0';
    output_value = ltsv_parse_string(file_contents);
    free((void*)file_contents);
    return output_value;
}

LTSV *ltsv_parse_string(const char *string) {
    if (!string) { return NULL; }
    
    return parse_ltsv((const char**) &string);
}

const char * ltsv_record_get_value(const LTSV_Record *record, const char *key) {
    return ltsv_record_nget_value(record, key, strlen(key));
}

size_t ltsv_record_get_count(const LTSV_Record *record) {
    return record ? record->count : 0;
}

const char * ltsv_record_get_name(const LTSV_Record *record, size_t index) {
    if (index >= ltsv_record_get_count(record)) { return NULL; }
    return record->labels[index];
}

size_t ltsv_get_count(const LTSV *ltsv) {
    return ltsv ? ltsv->count : 0;
}

LTSV_Record * ltsv_get_record(const LTSV *ltsv, size_t index) {
    if (index >= ltsv_get_count(ltsv)) { return NULL; }
    return ltsv->records[index];
}

void ltsv_free(LTSV *ltsv) {
    while (ltsv->count--) { ltsv_record_free(ltsv->records[ltsv->count]); }
    free((void*)ltsv->records);
    free((void*)ltsv);
}

