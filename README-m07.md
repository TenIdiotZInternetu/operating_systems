# Milestone 07: Userspace Applications

The goal of this milestone is to provide support for executing user
mode applications (with reduced privileges) under the supervision of
your kernel. By application we really mean a simple process with its own
virtual address space and a single (main) thread.

In addition to modifications to your kernel, you will also need to
create an environment in which the user mode process can execute -- the
so called user space. This in turn requires creating at least a rudimentary
standard C library (`libc`) which will provide the application with
services such as memory allocation and console output backed by kernel
services accessible via system calls.

This is the final milestone: you have started with nothing more than few
instructions that bootstrapped the CPU into your C code and after this
milestone you will be able to run trivial C applications containing the
traditional `main()` function. Great job!

---

## General notes

This milestone builds on top of M06, because user-mode applications can
only access virtual memory pages
marked as _accessible to user space_ (the `U` flag in their page table entry
must be set but we expect that `sstatus.SUM` bit set for simplicity).
The interface for creating user processes is simple, but sufficient for a
proof-of-concept implementation of user space.

To create a process, you will need to implement the `process_create`
function in which you provide an output argument for the `process_t`
structure handle and the address of the application image and its size.
The function will set up the virtual address space and launch the
process.

The binary application image will be loaded by MSIM to a fixed memory
location from which you are expected to copy it to the virtual memory of
the new process. Note that because the image is raw and includes static
data, you cannot just map it to the address space, because multiple
processes using the same image would be sharing writable data.

The virtual memory layout is trivial. As in M06, the first two virtual
memory pages (addresses from `0` to `8191`) are left unmapped, which
allows detecting a `NULL` pointer dereference. The next two pages are
reserved for the stack, followed by the application image (with static
data) and (optionally) heap.

Because we only need to support single-threaded applications, placing
the stack right after the unmapped part of the virtual address allows
detecting stack overflows. Supporting more threads would be possible
if the stacks were allocated dynamically, but the virtual memory management
would have to support address spaces with multiple virtual memory areas
to retain the ability to detect stack overflows.

To do something useful, the application needs to be able to ask the
kernel to perform various (privileged) operations on its behalf. From
the perspective of an application, these are just calls to library
functions. These are implemented using system calls, and as such very
much depend on the system call interface of a particular kernel. You
will therefore need to create your own (minimal) standard C library
(`libc`), which will provide functions for console output, heap
management, and process termination.

Note that when a user process tries to do something it is not supposed
to do, (e.g., access kernel memory), it should be killed. On the other
hand, providing wrong arguments to a system call should be communicated
back in a non-violent form (error codes).

To simplify reasoning about contents of `app.bin` (i.e. the plain application
binary to be executed in your environment), it is padded with zeros at
the beginning to emulate the empty space at first pages (see `app.lds` if
you are interested in details). There is no need to copy the zeros but make
sure the area is copied with the correct shift (offset).

---

## Kernel and user-space stacks

The code running in user mode requires the stack to be in a user-accessible
memory (accessing kernel memory will cause an exception). Even though this is
a natural requirement for user-mode code, it can cause trouble when executing
kernel code in the context of a user-mode thread. This happens on every CPU
exception, be it a timer interrupt, system call, or a page fault.

Recall that whenever an exception occurs, the CPU switches to a more privileged
execution mode and jumps to the exception handler. The low-level part of the
handler (written in assembly) needs to store the context of the interrupted
thread before executing the high-level part of the handler (written in C).
The context is stored on the stack of the interrupted thread. This makes the
exception handler reentrant, which is generally desired, even though we do not
make use of it (it would be needed if we wanted the kernel to be able to,
e.g., reschedule a thread during a lengthy system call).

Storing the thread context on the stack works without problems for kernel
threads: the memory is (usually) directly mapped and the kernel code is not
expected to require large stacks. However, if a user thread overflows its
stack it triggers a page fault. If the handler tried to store the context on
the stack that just overflowed, it would cause another (nested) exception,
jumping to the handler again and so on.

In general, recovery from nested exceptions is difficult and requires
the exception handlers to be fully reentrant. This means that we need to
be able to save information about the interrupted context before another
exception can be handled.

Because such situations are best avoided, a common solution is to use
**two** stacks: one for user mode and one for kernel mode (alternatively
one for normal execution and another one for exception handling). With
two stacks, the exception handler can always switch to "safe" kernel
stack before saving the context.

For this milestone, we have extended the low-level exception handlers
to switch to kernel stack whenever we interrupt a thread running in user
mode, using the value stored in the `kernel_stack_top` variable. When
the exception handler returns, it restores the original stack. To make
this transparent, we update the `kernel_stack_top` variable in the
`cpu_switch_context` function on each context switch. You just need to
allocate kernel stack even for user mode threads.

Note that cooperative context switching performed by
`cpu_switch_context()` always uses kernel memory, because it stores the
context somewhere in the `thread_t` structure. Actually, even this function
could store the context on the stack (provided each thread "remembers" the
stack top in `thread_t`), but the interface to `cpu_switch_context()` would
become much less straightforward (requiring a pointer to a pointer to stack
top). We believe that storing the cooperative context separately makes
context switching easier to understand.

Finally, note that the separate stacks also increase security. Kernel
code executing with user stack could leave stack frame fragments on the
user stack, potentially leaking private information. Switching to kernel
stack prevents such issues by keeping the kernel stack frames in kernel
memory.

---

## Basic functions

A new user process is created using the `process_create()` function; waiting
for the process to finish is done via `process_join()`. The main process
thread should be created by calling the respective `thread_create*()` function
inside the `process_create()` function. Feel free to use the (currently unused)
`flags` parameter of `thread_create*()` to indicate whether the thread is
supposed to run in kernel or user mode.

The transition to user mode is implemented in the `cpu_jump_to_userspace()`
function, which sets up the program counter, stack pointer, and switches the
CPU to user mode while jumping to the user entry point. This means that user
mode threads should start in kernel mode and use the `cpu_jump_to_userspace()`
function to switch to user mode.

In user space, it is necessary to add at least an implementation of the
`putchar()` function to enable console output. The function can be implemented
either by using a suitable system call, or by writing to a printer device that
has been mapped into user-accessible region of the address space.

Under normal circumstances, a user process terminates by invoking the `exit`
system call. From the perspective of the CPU, a system call is just another
exception which is handled by the general exception handler (in addition to
other exceptions that a process may cause). A system call exception does not
require restarting the instruction that caused the exception (unlike, e.g.,
a page fault exception, during which the operating system can map a virtual
memory page on demand and restart the offending instruction after updating
page tables or TLB). Therefore when servicing a system call, the handler must
manually shift the exception program counter (EPC) to the next instruction.

In summary, to provide basic functions to user-mode processes, we expect each
team to implement at least the `exit` system call and the `putchar()` function.


### User application

The actual user application is written inside the `main()` function. Similar to
threads, its return value (of type `int`) is used as an exit code of the whole
application. The value can be also set by calling the `exit(int)` function that
sets the exit code and terminates the application immediately.

The skeleton code for this milestone can execute C code inside the `__main()`
function. The kernel part is handled by the `cpu_jump_to_userspace()` function
that will jump to the user process entrypoint (at the `PROCESS_ENTRY_POINT`
address) where the assembly code jumps into C `__main()` which is responsible
for setting up the process environment in user space. It is your responsibility
to call the user `main()` function from there.


### Basic library: `libc`

The C standard defines about 20 header files that each standards-compliant
`libc` must provide: they should contain several hundreds of symbols to
provide the fundamental functions for C programs (and POSIX adds a few on
top of these).

You are required to implement only a subset that is needed to get the tests
running. All standard functions are placed in the usual headers (e.g. you
will find `exit()` in [`stdlib.h`](userspace/libc/include/stdlib.h) or
`putchar()` in [`stdio.h`](userspace/libc/include/stdio.h)), extensions
are inside the non-portable [`np`](userspace/libc/include/np) subdirectory
(e.g. `utest_assert()` used in milestone tests is inside
[`np/utest.h`](userspace/libc/include/np/utest.h)).

Note that implementing the entire C library (respecting the C11 standard) is
no small task: many of the functions require kernel support that is (by orders
of magnitude) outside the scope of the simple kernels we are building during
this course (consider, e.g., support for working with files). But it is the
last step before actual applications can be compiled: even managed runtime
environments (e.g., Java, or Python) are built on top of `libc` in some way.


## Optional extensions

One straightforward extension of the baseline solution is providing a user-mode
`printf()` function. A basic implementation can copy the kernel `printk()`
function (without the extra `%p` modifiers) that relies on single-character
output fuction. An advanced user-mode `printf()` can buffer the output before
sending it to kernel in larger chunks (if `putchar()` is implemented using a
system call).

To enable dynamic memory allocation in user space, the library can provide
the `malloc()` and `free()` functions that operate on a **user-space heap**.
To find out the (user) heap starting address, you can use the `_app_end` symbol
(defined by the linker) the same way the `_kernel_end` symbol is used by the
`kernel_get_image_end()` function in the [`mm/self.h`](kernel/include/mm/self.h)
header file. Together with the information about the size of the process
virtual memory, you should have all that is needed to port your kernel heap
allocator to user space.

Finding out information about a process, including the size of its virtual
memory, is the responsibility of the `np_proc_info_get()` function which
fills the `np_proc_info_t` structure (some of the information it provides is
only used for testing). We expect you to use a single system call to
fill this structure.

---

## Support code

As usual, new milestone comes with new code to be merged into your kernel,
including some changes to existing code. Here we would like to point out the
change to kernel initialization, which starts a user mode application by
default, unless the kernel is compiled to execute a kernel test.

### Tests

Without any extra parameters to `configure.py`, a trivial user application
is started. By using the `--userspace-test` parameter, we configure the user
application to execute a specific user-mode test. These tests check the
implementation of various parts of the `libc` library.

All tests covering the user-space functionality are inside `userspace/tests`.
Note that many of them check if an application is killed when it tries to
perform some privileged operation.

The `tester.py` (and `configure.py`) scripts now support three types of tests:
`kernel` tests that were used so far (there is no change in their invocation),
`userspace` tests (`--userspace-test`) where no kernel test is run but a single
user application is started with a given test, and finally `mixed` tests
(`--mixed-test`) that have more complex setup of the kernel test as well as of
the userspace part. These are used for example to stress test your
implementation by spawning multiple userspace applications (they are all the
same, but their virtual memory will be backed by different physical frames).


### Hints

We highly recommend to start with your kernel configured without any tests
(run `configure.py` without any further settings, except perhaps `--debug`)
and put a breakpoint (see `machine_enter_debugger()`) inside the `__main()`
function in user space [`main.c`](userspace/libc/src/main.c). This will cause
MSIM to enter interactive mode once your kernel manages to jump to user space.

We have also provided a skeleton of the `handle_syscall()` function. It is
not called from anywhere yet, but it should give you some idea of what needs
to be done when handling system calls.


---

## Additional hints/suggestions

* Where will a syscall be intercepted?
* What syscalls would be needed for printing to console? Should
  `printf()` be a syscall? Or should everything be routed through some
  kind of `putchar()` syscall?
* Who will be responsible for populating the virtual memory with the
  application image?
* Why we cannot detect memory the same way we do it in kernel (in M02)?
* Will the interrupts be disabled during syscall handling?

---

## Submission

The **baseline solution** for this milestone must pass the baseline tests for
`M01`, `M02`, `M03`, `M04`, `M05`, `M06` and `M07`. The commit representing
the solution must be tagged with `m07`.
Submitting a **working baseline solution** within this milestone's deadline
allows you to gain bonus points towards a better grade.

Extended features are usually covered by separate test suites (see comments on
the first line of the respective suite for details). Some features cannot be
reliably tested and will be evaluated during final review.

Please, see also the notes in the main [README.md](README.md).
