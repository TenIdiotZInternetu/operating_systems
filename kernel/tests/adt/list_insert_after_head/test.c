#include <adt/list.h>
#include <ktest.h>
#include <ktest/list_checking.h>

void kernel_test(void) {
    ktest_start("adt/list_insert_after_head");

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