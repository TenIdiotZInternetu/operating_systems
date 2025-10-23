// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <adt/list.h>
#include <ktest.h>
#include <ktest/list_checking.h>

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
