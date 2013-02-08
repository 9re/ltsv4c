#ifndef ltsv4c_h
#define ltsv4c_h

#include <stddef.h>   /* size_t */    

#ifdef __cplusplus
extern "C"
{
#endif    

typedef struct ltsv_record_t LTSV_Record;
typedef struct ltsv_t        LTSV;

LTSV *ltsv_parse_file(const char *filename);
LTSV *ltsv_parse_string(const char *string);

/* record operation apis */
size_t ltsv_record_get_count(const LTSV_Record *record);
const char * ltsv_record_get_name(const LTSV_Record *record, size_t index);
const char *ltsv_record_get_value(const LTSV_Record *record, const char *key);

/* ltsv operation apis */
size_t ltsv_get_count(const LTSV *ltsv);
LTSV_Record * ltsv_get_record(const LTSV *ltsv, size_t index);
void ltsv_free(LTSV *ltsv);

#ifdef __cplusplus
}
#endif

#endif
