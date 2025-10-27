#include <adt/list.h>
#include <ktest.h>

typedef struct {
    int value;
    link_t link;
} item_t;

static void item_init(item_t* item, int value) {
    link_init(&item->link);
    item->value = value;
}

#define CHECK_LIST_CONTENTS(size, list, ...) \
    do { \
        ktest_assert(list_get_size(&list) == size, "list not " #size " but %d", list_get_size(&list)); \
        int _expected_values[] = { __VA_ARGS__ }; \
        if (size > 0) { \
            int _i = 0; \
            list_foreach(list, item_t, link, _it) { \
                ktest_assert(_it->value == _expected_values[_i], \
                        "[%d]: not %d but %d", _i, _expected_values[_i], _it->value); \
                _i++; \
            } \
        } \
    } while (0)
