// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <assert.h>
#include <np/machine.h>

extern int main(void);
extern void __main(void);

/** First C code running in userspace. */
void __main() {
    int code = main();
    exit(code);

    assert(0 && "unreachable code");
}
