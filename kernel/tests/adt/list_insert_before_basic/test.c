#include <adt/list.h>
#include <ktest.h>
#include <ktest/list_checking.h>

void kernel_test(void) {
    ktest_start("adt/list_insert_before_basic");

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
