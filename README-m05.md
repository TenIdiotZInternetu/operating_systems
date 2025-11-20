# Milestone 05: Preemptive Scheduler and Synchronization

The goal of this milestone is to extend your kernel with support for
preemptive thread scheduling and with basic synchronization primitives
as optional extensions. Once implemented, threads will not need to
explicitly *yield* the processor to other threads to achieve multitasking
--- the kernel will switch between threads periodically in response to
interrupts from a timer device.

This may appear to be a relatively simple change, but it has far-reaching
consequences. In general, whenever a thread works with shared data, it must
ensure that its actions do not conflict with the actions of other threads.

Achieving this is easier when threads are scheduled cooperatively, because
the running thread determines when other threads can run --- it can
simply avoid yielding the processor to other threads while in the middle
of a sensitive non-atomic operation (critical section), e.g., connecting
pointers when adding or removing item to a linked list.

It is important to realize that with preemptive scheduling the currently
running thread is **no longer in control** ---  it can be **interrupted
and rescheduled at any time**, i.e., even in the middle of a critical
section (resulting in data corruption or some other erroneous behavior).
In this context, threads need some means to access shared data in an
orderly fashion, hence the need for synchronization primitives.

Even though the milestone is split into two parts (to reflect the
logical distinction between scheduling and synchronization), preemptive
scheduling and synchronization really go hand-in-hand.

---

## Part 1: Preemptive Scheduler

To perform preemptive scheduling, your kernel needs to switch between threads
without waiting for the currently running thread to yield the processor. But
for this to happen, the processor needs to be executing the kernel code which
implements the context switching, i.e., it needs to somehow interrupt whatever
the current thread is doing.

This is typically achieved with the help of a timer device, which will
periodically issue an interrupt request to the processor. When interrupt
processing is enabled on the CPU, a timer interrupt causes the CPU to jump
into an interrupt handler code of the kernel, i.e., it will stop executing the
code of the currently running thread. In the timer interrupt handler code, the
kernel can invoke the scheduler to forcibly switch context to another thread.

Even though the RISC-V processor provides a real-time timer, we will use a
simple MIPS-like cycle-based timer to make execution fully deterministic.
Your scheduler code will need to configure the timer device using the
`timer_interrupt_after()` function defined in the
[`drivers/timer.h`](kernel/include/drivers/timer.h) header file, which
allows setting the interval until next timer interrupt (see the section
on [timer control](#timer-control) for more details).

Your kernel and scheduler will need to be modified to use the timer to
implement preemptive scheduling. Because we do not require your kernel to
provide a generic timer support, it is possible to program the timer device
directly from the scheduler --- no other part of your kernel will use it.

You will also need to trigger execution of the scheduler code whenever the
timer interrupts the currently running thread. To this end, you will
need to modify the general exception handler to recognize and properly
handle the timer interrupt, and to trigger the scheduler code. See the
section on [trap handling](#trap-handling) for more details.

### Configurable quantum

Preemptive scheduler operates with the concept of a *quantum*, which
determines the maximum time a thread is allowed to run before the
scheduler unconditionally switches to another thread. A shorter
*quantum* causes threads to be switched faster (making the system more
responsive) at the cost of higher overhead due to context switching.

The value of *quantum* also influences the interleaving of threads, which
makes it useful for testing, because different values can lead to different
thread schedules and help expose race conditions.

For this reason, your scheduler should use the `KERNEL_SCHEDULER_QUANTUM`
macro, which was added to [`proc/scheduler.c`](kernel/src/proc/scheduler.c),
as the value of a thread quantum. This will make it possible to test your
code with different quanta, which is exactly what the tests provided with
this milestone do.

Real kernels are usually configured with quantum specified in milliseconds.
To ensure deterministic executions we will always define quantum based on
instruction count (i.e. you configure the timer to raise the interrupt after
kernel executes `KERNEL_SCHEDULER_QUANTUM` instructions).

---

## Part 2: Synchronization primitives (optional extensions)

There are many synchronization primitives, but the optional part of this
milestone only concerns two of them. The first is *mutex*, declared in the
[`proc/mutex.h`](kernel/include/proc/mutex.h) header file, which allows
implementing *mutual exclusion*, the most basic synchronization task.

The second is a *semaphore*, declared in the [`proc/sem.h`](kernel/include/proc/sem.h)
header file, which allows multiple threads to enter a critical section at the
same time. Semaphores are a bit more complex, and are used for synchronization
tasks involving a fixed number of resources, e.g., producer/consumer
with a bounded buffer.

Just like in previous milestones, the header files prescribe the API
that you are required to implement. Both mutexes and semaphores must
use *passive waiting*. That, however, requires managing sleeping threads.
You may want to start with simpler, active-waiting variants, which do
not require thread management.

---

## Thread safety

All kernel code that can execute under preemptive scheduling **must be made
thread-safe**. Specifically, any function (e.g., `kmalloc()` and `kfree()`)
that works with data that can be accessed concurrently from multiple threads
must synchronize access to the shared data. In addition, some interactions
between subsystems must be atomic.

This can result in a chicken-and-egg problem, because synchronization
primitives such as mutexes or semaphores rely on the thread and scheduler
subsystems, which means they cannot be used to implement, e.g., mutual
exclusion, in those subsystems. This also applies to the implementation of
the synchronization primitives themselves.

In such cases, you will need at least a rudimentary form of locking, which
can be (at least initially) implemented by temporarily disabling interrupts,
which is used in the following examples. See the section on
[low-level mutual exclusion](#low-level-mutual-exclusion) for more details.

To illustrate the required changes, consider the following code which
always updates the `pre` member of a shared data structure, but updates
the `post` member only when `update_post` is `true`:

```c
void do_something_with_shared_data(bool update_post) {
    shared_data.pre = shared_data.pre + 1;
    if (!update_post) {
        return;
    }
    shared_data.post = shared_data.post + 1;
}
```

The code works just fine in a cooperatively-scheduled system, but is not
thread-safe in a preemptively-scheduled system. We can protect the shared data
structure by disabling interrupts as follows:

```c
void do_something_with_shared_data(bool update_post) {
    bool saved = interrupts_disable();

    shared_data.pre = shared_data.pre + 1;
    if (!update_post) {
        interrupts_restore(saved);
        return;
    }
    shared_data.post = shared_data.post + 1;

    interrupts_restore(saved);
}
```

Note that the above code interleaves two responsibilities (locking and data
manipulation), which makes the code more complicated, because you need to keep
track of early returns to issue the call to `interrupts_restore()` before
returning.

To reduce code complexity in such situations (especially with pair operations
such as mutex lock/unlock or disable/enable interrupts), we recommend to keep
the two responsibilities separate by splitting the code into two functions
--- one responsible for the pair operation and the other responsible for
the actual operation on a data structure, as follows:


```c
static void unsafe_do_something_with_shared_data(bool update_post) {
    shared_data.pre = shared_data.pre + 1;
    if (!update_post) {
        return;
    }
    shared_data.post = shared_data.post + 1;
}

void do_something_with_shared_data(bool update_post) {
    bool saved = interrupts_disable();

    unsafe_do_something_with_shared_data(update_post);

    interrupts_restore(saved);
}
```

---

## Support code

Just like the previous milestones, this milestone updates the kernel
base code to provide the necessary context and some convenience
functions. We recommend to review the update to get an idea what has
changed and where.


### Low-level mutual exclusion

Without synchronization primitives such as mutexes and semaphores, mutual
exclusion can be implemented as *active waiting* by (repeatedly) checking
the value of a variable using a *test-and-set* operation in a loop. Such
an implementation is usually referred to as a *spin-lock*. However, because
your kernel is only expected to run on a single processor, you can use an
even simpler approach, which relies on temporarily disabling interrupts.

Disabling interrupts means that the processor *ignores* interrupt requests,
which allows the currently executing thread to run through a (critical)
section of code without being interrupted and rescheduled (recall that
the scheduler relies on timer interrupts to switch between threads
unconditionally). After reaching the end of the critical section, interrupts
need to be enabled again to resume normal preemptive scheduling.

Control over interrupts is provided by the `interrupts_disable()` and
`interrupts_restore()` functions defined in the [`exc.h`](kernel/include/exc.h)
header file. The `bool interrupts_disable()` function returns the CPU
interrupt-processing state prior to disabling interrupts, while the
`void interrupts_restore(bool)` function restores the interrupt-processing
state to the given value. This allows using the functions in nested calls.
See the section on [interrupt control](#interrupt-control) for more details.

**Note:** disabling interrupts does not make the interrupt requests go away
--- if an interrupt request is pending, you can expect it to interrupt
the processor the moment interrupt processing on the processor is enabled.

**Note:** the interrupt-processing state is part of the thread context (similar
to CPU registers), i.e., disabling interrupts in one thread has no impact on
the interrupt-processing state in other thread. A (kernel) thread may disable
interrupts and switch to another thread in which interrupt-processing is
enabled. This is a feature, not a bug.


### Trap handling

The most important change is in trap (exception) handling. The code in the
`start` kernel entry point in [`head.S`](kernel/src/head.S) configures the
processor to execute the kernel in *supervisor* mode (S-mode) and to handle
exceptions related to timer interrupts (also page faults, system calls, and
invalid instructions) in an S-mode trap handler (`supervisor_trap_handler` in
[`exc/handlers.S`](kernel/src/exc/handlers.S)).

The handler saves the complete context (exception context) of the interrupted
thread on the stack and calls the `handle_exception_general()` function defined
in [`exc/exc.c`](kernel/src/exc/exc.c), which is responsible for handling general
exceptions (including interrupts).

Any other exceptions will still be handled by the *machine* mode (M-mode) trap
handler (`machine_trap_handler` in [`head.S`](kernel/src/head.S)) which just
dumps the CPU registers and switches MSIM to interactive mode, because they
should not really occur.

The `handle_exception_general()` function receives a single argument of type
`exc_context_t *`, which is a pointer to *exception context* containing the
values of CPU registers at the time of exception (interrupt) along with
important control registers that need to be preserved. The exception context
is defined in the [`exc/context.h`](kernel/include/exc/context.h) header file.

The exception handler should inspect the value of the *Supervisor Exception
Cause* (`scause`) register in the exception context to determine the cause
of the exception (with the help of functions defined in the
[`drivers/csr.h`](kernel/include/drivers/csr.h) header file) and to handle
it accordingly.

To implement preemptive scheduling you should only need to handle exceptions
caused by interrupts. For any other causes you should dump the CPU and
relevant control registers and switch the simulator to interactive mode to
simplify debugging (other causes include misaligned memory accesses or
similar issues that accompany various bugs).



### Timer control

Even though the RISC-V processor provides a real-time timer, we will not use
it, because it would introduce non-determinism into the execution of your kernel
in the simulator, making it impossible to deterministically reproduce
synchronization-related bugs.

Instead, MSIM implements a non-standard RISC-V extension which introduces a
cycle-based timer interrupt, which can be configured through the `cycle` and
`scyclecmp` CSRs. Because `scyclecmp` is not a standard CSR, the name will
not be recognized by the assembler, but it can be accessed via its CSR
number: `0x5C0`.

The `cycle` CSR acts as a timer and is being incremented by the CPU at a
constant rate. The value of the `scyclecmp` register does not change on its
own, but whenever `cycle` equals `scyclecmp`, the processor issues an
interrupt request by setting the *Supervisor Timer Interrupt Pending* bit
(`sip.STIP`) of the *Supervisor Interrupt Pending* (`sip`) register. This
triggers an interrupt soon as the interrupts are enabled, which causes the
processor to jump into the general exception handler.

The `scyclecmp` CSR can be read and written by kernel code, but `cycle` is
read-only. Writing a value larger than that of `cycle` to `scyclecmp` clears
the supervisor timer interrupt.

The [`drivers/timer.h`](kernel/include/drivers/timer.h) header file provides
very basic support for this timer. Because your scheduler should only need to
set the interval until the next timer interrupt, the provided
`timer_interrupt_after()` function does exactly that.

Specifically, the function sets the value of the `scyclecmp` CSR to the
current value of the `cycle` CSR incremented by the given number of CPU
cycles. The number of cycles determines the interval until next timer
interrupt.

**Note:** when dumping the contents of `sip`, MSIM displays `STIP` and
`External STIP`. The value of `sip.STIP` read by software is the *logical OR*
of these values, while software writes to `sip.STIP` write to `STIP` and
not to `External STIP`. The interrupt caused by `scyclecmp` actually sets
the `External STIP`. This behavior is the same for `SEIP` and `External SEIP`,
and is defined in this way to separate the values written by hardware and
software.


### Interrupt control

Interrupts on RISC-V are controlled by *Supervisor Interrupt Enable* (`sie`)
and *Supervisor Status* (`sstatus`) registers. Specifically, bit 1 of the
`sstatus` register (*Supervisor Interrupt Enable*, `sstatus.SIE`) controls
the CPU interrupt-processing state for all interrupts. The `sie` register
allows masking (disabling) individual interrupts.

When the `sstatus.SIE` bit is cleared, the CPU ignores all interrupts. When
the bit is set, the CPU reacts to interrupts that are enabled by the
individual bits in the `sie` register.

The [`exc.h`](kernel/include/exc.h) header file provides two simple functions
for coarse-grained interrupt control, i.e., these functions only manipulate
the state of the `sstatus.SIE` bit of the *Supervisor Status* register.


### Debugging

We have extended the tester script to also check that code that should
panic the kernel actually panics. This is needed for example in the
`mutex_destroy` test, which checks that the kernel panics when trying to
destroy a locked mutex.

As usual, some of the tests use extended `%p` specifiers to print
details about certain kernel objects. We use `%pS` for semaphores and
`%pM` for mutexes (which is why we used `%pA` for allocated memory block
in the previous milestone). These `printk()` extensions are optional
and do not affect the test outcome.


### Tests

The tests provided in this milestone can be split into two groups:
tests for the synchronization primitives and extensions to the existing
tests to check that preemptive scheduling does not break your
implementation.

The synchronization tests check different aspects of the primitives and
should be easy to understand. The extended tests (for heap and threads)
check that the scheduler is really preemptive and that the internal data
structures are not corrupted by concurrent execution.

Because preemptive scheduling provides a lot of room for race conditions
which may only manifest under a specific thread schedule, we have added
a `KERNEL_SCHEDULER_QUANTUM` macro to [`proc/scheduler.c`](kernel/src/proc/scheduler.c)
which determines the duration of the thread time quantum. Kernel tests
can use an additional parameter (`kqNUM`, see [`suites/m05-sem-fuzzy.txt`](suites/m05-sem-fuzzy.txt))
to change this quantum from [`tools/tester.py`](tools/tester.py). This
is extremely useful for flushing out race conditions that are bound to
appear.

Note that the provided fuzzy suite is meant to be run for every commit
and is relatively short. Consider creating a custom test suite with more
fine-grained steps (and for multiple tests) to properly stress-test your
solution. However, because such a test suite can run several hours,
please **DO NOT** integrate it into your CI configuration and only run
it locally.

As usual, the tests should help you check that you have not forgotten
any major part of the implementation. It is possible to pass some tests
even with a solution that does not really work.

While passing the tests is required for this milestone, your
implementation will also be checked manually, and must include working
scheduler and thread management functions to actually pass.

---

## Additional hints/suggestions

  * `sip.STIP` is bit number 5 of `sip`.
  * What happens when `handle_exception_general()` returns?
  * Why do we need `interrupts_restore()` (as a counterpart to
    `interrupts_disable()`) instead of `interrupts_enable()`?

---

## Submission

The **baseline solution** for this milestone must pass the baseline tests for
`M01`, `M02`, `M03` and `M04` and `M05`. The commit representing the solution
must be tagged with `m05`. Submitting a **working baseline solution** within
this milestone's deadline allows you to gain bonus points towards a better
grade.

Extended features are usually covered by separate test suites (see comments on
the first line of the respective suite for details). Some features cannot be
reliably tested and will be evaluated during final review.

Please, see also the notes in the main [README.md](README.md).
