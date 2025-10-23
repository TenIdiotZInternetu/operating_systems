// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

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

void kernel_test(void) {
    ktest_start("adt/list2");

    item_t a, b, c, d;
    item_init(&a, 1);
    item_init(&b, 2);
    item_init(&c, 3);
    item_init(&d, 4);

    list_t items;
    list_init(&items);
    CHECK_LIST_CONTENTS(0, items);

    list_append(&items, &a.link);
    CHECK_LIST_CONTENTS(1, items, 1);

    list_append(&items, &b.link);
    CHECK_LIST_CONTENTS(2, items, 1, 2);

    list_prepend(&items, &c.link);
    CHECK_LIST_CONTENTS(3, items, 3, 1, 2);

    list_append(&items, &d.link);
    CHECK_LIST_CONTENTS(4, items, 3, 1, 2, 4);

    link_t* rotated = list_rotate(&items);
    CHECK_LIST_CONTENTS(4, items, 1, 2, 4, 3);

    item_t* item = list_item(rotated, item_t, link);
    ktest_assert(item->value == 3, "rotated item is not 3 but %d", item->value);

    list_remove(&b.link);
    CHECK_LIST_CONTENTS(3, items, 1, 4, 3);

    link_t* popped = list_pop(&items);
    CHECK_LIST_CONTENTS(2, items, 4, 3);
    item = list_item(popped, item_t, link);
    ktest_assert(item->value == 1, "popped item is not 1 but %d", item->value);

    ktest_passed();
}


/***************************************
 * Tests: list_insert_after()
 ***************************************/

void test_insert_after_basic() {
    list_t list;
    list_init(&list);

    item_t a, b, c;
    item_init(&a, 1);
    item_init(&b, 2);
    item_init(&c, 3);

    list_append(&list, &a.link); // list: 1
    list_insert_after(&list, &a.link, &b.link); // list: 1, 2
    list_insert_after(&list, &b.link, &c.link); // list: 1, 2, 3

    CHECK_LIST_CONTENTS(3, list, 1, 2, 3);
    ktest_passed();
}

void test_insert_after_head() {
    list_t list;
    list_init(&list);

    item_t a, b;
    item_init(&a, 1);
    item_init(&b, 2);

    list_insert_after(&list, &list.head, &a.link);
    list_insert_after(&list, &a.link, &b.link);

    CHECK_LIST_CONTENTS(2, list, 1, 2);
    ktest_passed();
}

void test_insert_after_prev_link_not_connected_appends_links() {
    list_t list;
    list_init(&list);

    item_t a, b, c;
    item_init(&a, 1);
    item_init(&b, 2);
    item_init(&c, 3);

    list_append(&list, &a.link);

    list_insert_after(&list, &b.link, &c.link);

    CHECK_LIST_CONTENTS(3, list, 1, 2, 3);
    ktest_passed();
}

void test_insert_after_empty_list_appends_links() {
    list_t list;
    list_init(&list);

    item_t a, b;
    item_init(&a, 1);
    item_init(&b, 2);

    list_insert_after(&list, &a.link, &b.link);

    CHECK_LIST_CONTENTS(2, list, 1, 2);
    ktest_passed();
}


/***************************************
 * Tests: list_insert_before()
 ***************************************/

void test_insert_before_basic() {
    list_t list;
    list_init(&list);

    item_t a, b, c;
    item_init(&a, 1);
    item_init(&b, 2);
    item_init(&c, 3);

    list_append(&list, &a.link); // list: 1
    list_append(&list, &c.link); // list: 1, 3
    list_insert_before(&list, &c.link, &b.link); // list: 1, 2, 3

    CHECK_LIST_CONTENTS(3, list, 1, 2, 3);
    ktest_passed();
}

void test_insert_before_head() {
    list_t list;
    list_init(&list);

    item_t a, b;
    item_init(&a, 1);
    item_init(&b, 2);

    list_insert_before(&list, &list.head, &b.link);
    list_insert_before(&list, &b.link, &a.link);

    CHECK_LIST_CONTENTS(2, list, 1, 2);
    ktest_passed();
}

void test_insert_before_empty_list_appends_links() {
    list_t list;
    list_init(&list);

    item_t a, b;
    item_init(&a, 1);
    item_init(&b, 2);

    list_insert_before(&list, &b.link, &a.link);

    CHECK_LIST_CONTENTS(2, list, 1, 2);
    ktest_passed();
}