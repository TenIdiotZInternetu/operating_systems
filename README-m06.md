# Milestone 06: Virtual Memory Management

After a short break in the form of thread support, preemptive scheduling, and
synchronization primitives, this milestone returns to memory management. The
main goal of this milestone is to extend your kernel with basic support for
virtual memory, which will be needed for user-space threads and processes.

Without full support for virtual memory the kernel is not able to guarantee
that userspace applications will not read each other memory: only by adding
proper page table translation we can create the illusion that an application
running in userspace owns all the memory (and also cannot access any other
memory). We will add support for running userspace applications in the next
milestone but we will prepare the page tables in this milestone.

Implementing support for virtual memory requires managing the address
translation hardware (usually referred to as _memory management unit_, MMU) and
servicing exceptions related to address translation that are bound to occur
when threads access virtual memory. This in turn requires the kernel to support
the notion of a virtual address space backed by a physical memory.

Translation of virtual to physical addresses is generally performed with the
granularity of fixed-size blocks (instead of individual bytes) so that the
hardware can implement it **efficiently**. In the virtual address space, the
blocks are called (virtual) _pages_, while in the physical address space, the
blocks are called (physical) _frames_. In some contexts, they may be referred
to as _virtual pages_ and _physical pages_.

Regardless of the name, both pages and frames have equal size, and are
**naturally aligned** (i.e., they start at addresses divisible by their size).
This is necessary to allow the address translation hardware to map the part of
the virtual address representing a virtual page number to a corresponding
physical frame number and then just append the bits representing the offset in
the virtual page to the physical frame number, producing a physical address
(more on [address translation](#address-translation) later).

We have already introduced frame allocator in one of the earlier milestones.
In this milestone we will use it give us frames that will be backing the
virtual pages.

---

## Address spaces

In general-purpose operating systems, an address space is a collection of
virtual memory areas (VMA) which represent ranges of valid virtual addresses
that a program can use. The virtual memory areas typically correspond to areas
known from process memory layout, i.e., code, static data, heap, and thread
stack(s), but there can be more (think of dynamically linked libraries). The
operating system is responsible for establishing the mapping of (valid) virtual
memory addresses to physical memory addresses (with the granularity of pages).

Your task in this milestone will be simpler, because we require your
implementation to only support a **single** VMA. The size of the VMA will be
fixed at creation time, and it will always start at virtual address
`2 * PAGE_SIZE`. This means that the first two pages (addresses from `0` to
`2 * PAGE_SIZE - 1`) **WILL NOT** be mapped, which ensures that dereferencing
a `NULL` pointer causes an exception.

Threads will be associated with address spaces. When creating a new
thread, the creator can choose whether to share its own address space
with the thread being created, or whether to create a thread with a new
address space of given size. To this end, the thread API in
the [`proc/thread.h`](kernel/include/proc/thread.h) header includes two new
functions that need to be implemented in
[`proc/thread.c`](kernel/src/proc/thread.c).

The kernel should destroy an address space when the **last** thread using it
terminates. This implies that the kernel needs to keep track of the number of
threads using a particular address space.

The functions in the [`mm/as.h`](kernel/include/mm/as.h) header file represent
the interface you must to implement (in [`mm/as.c`](kernel/src/mm/as.c)) to
enable address space management. You are expected to extend the `as_t`
structure as necessary to suit the needs of your implementation.

---

## Address translation


The actual address translation on RISC-V is done via page tables arranged
in a radix tree. The software is responsible for setting up the hierarchical
data structure, and the hardware uses it to perform address translation. So
far your kernel has been executing (and accessing memory) with the processor
set to run in _bare mode_, in which virtual addresses are identical to physical
addresses. This is a common processor feature, often referred to as
_identity mapping_, which simplifies kernel bootstrap, and which is why you did
not need to worry about setting up and managing the page tables so far.

In this milestone, though, you need to implement proper virtual memory support
which provides both flexibility in mapping the virtual pages to physical frames
and memory protection, which cannot be achieved in bare mode with identity
mapping.

The page table layout is straightforward, so that hardware can traverse the
data structure efficiently using the virtual address that is being translated.
Parts of the virtual address are used as indices at the corresponding page
table level, and the actual page table entries contain extra flags that
indicate whether the mapping is valid and whether read/write/execute access
is allowed.

You will need to setup the page table for each address space so as to replicate
the identity mapping of all physical memory for the kernel code (the mapping
will be the same in all address spaces) and to create a unique mapping for the
single virtual memory area in each address space. It is completely fine to
setup the entire page table structure when an address space is created. You
should consider using _mega pages_ for the kernel mapping to simplify your
code (it will also reduce the amount of allocated memory).

Please consult the section on Page-Based 32-bit Virtual-Memory Systems
(starting on page 79) and the `satp` register (page 73) in the
[RISC-V Instruction Set Manual, Privileged Architecture](https://d3s.mff.cuni.cz/files/teaching/nswi200/202324/doc/riscv-spec-privileged.pdf)
for details.

To simplify debugging and also to make the tests simpler we expect that your
kernel will run with `SUM` bit in the status register set. This bit effectively
enables the kernel to access userspace pages even when the user bit is set.


### Hardware support for address spaces

The RISC-V MMU caches address translations obtained from page tables in a TLB
(which is not accessible to the software). To make the address translation more
efficient when running multiple processes with different virtual memory maps,
the RISC-V MMU can tag TLB entries with a 9-bit _address space identifier_
(ASID) to disambiguate TLB entries with identical virtual addresses. The ASID
of the current thread (process) is set together with the address of the root
page table in the `satp` register.

The preferred implementation of address translation is to use the ASID
to allow translation entries for different address spaces to coexist in
the TLB. This requires allocating/assigning an unused ASID to an address
space on creation, and reclaiming the ASID on address space destruction.

At the other end of the spectrum, the baseline solution allows implementing
address translation without ASIDs, which just requires flushing the entire
TLB on each address space switch.

There are also other alternatives --- see the website and individual test
suites for typical extensions. Due to the broad spectrum of possible solutions,
we will review this part manually and assign extra points based on the actual
implementation.

Note that because the number of ASIDs supported by the hardware is rather high,
intensive tests tend to run for quite a while. To reduce test execution time,
we have introduced the `HARDWARE_ASIDS_MAX` constant (set at compile-time)
which limits the number of ASIDs available in your code. In practice, this will
not limit the number of ASIDs supported by the hardware (MSIM), but your code
(and the tests) will assume that a lower number of ASIDs is available and
should start recycling the ASIDs earlier, thus reducing execution time.

Note also that even though the RISC-V architecture supports run-time detection
of the number of ASIDs supported by the processor, the MSIM simulator does not
support this feature (yet). Consequently, even on RISC-V, your code should
depend on the value of the above constant.

---

## Support code

This milestone again updates the kernel base code to provide the necessary
context and some convenience functions. We recommend to review the update to
get an idea what has changed and where.



### Exception handling

The exceptions for invalid memory accesses will be delivered to the
`handle_exception_general()` handler in [`exc/exc.c`](kernel/src/exc/exc.c).
If your implementation does not create virtual memory mappings lazily (on
demand), you are expected to simply kill the offending thread.

### Page table management

We provide a few helper macros and functions for page table configuration in
the [`drivers/sv23.h`](kernel/include/drivers/sv32.h) header file.

The `sv32_set_satp()` function only sets the `satp` register to the given
value, i.e. the combination of ASID, mode and physical address (frame number)
of the root page table.

The `sv32_set_satp_tlb_shootdown()` function sets the `satp` register as above,
but also issues the `SFENCE.VMA` instruction to completely invalidate TLB. You
will probably want to use this function in an implementation that does not
support ASIDs (all threads are using the same ASID).

Finally, the `sv32_tlb_shootdown()` function only invalidates the TLB. This
function should be called when an address space is destroyed.

Note that you can use `cpu0 ptd` and `cpu0 sptd` commands of the MSIM simulator
to dump page tables (to verify you constructed them correctly) and there is
also a pair of `cpu0 tr` and `cpu0 str` commands to perform a translation
interactively.

### Tests

The milestone provides new tests to help you drive the implementation.

The virtual memory mapping tests check that your threads can access
virtual addresses in the first few kilobytes of virtual memory and that
separate address spaces do not influence each other.

As usual, the tests should help you check that you have not forgotten
any major part of the implementation. It is possible to pass some tests
even with a solution that does not really work.

Note that the special test `as/leaks` tries to check that you are properly
returning memory when resource allocation partially fails. As in M03, use
`--debug` builds to print information about leaking resources.

---

## Additional hints/suggestions

* What is PPN and VPN0 and VPN1?
* Development hints and suggestions
    * Clearly mark variables containing physical addresses so that you do
      not treat them as regular pointers (and never store them as `void *`).
    * Frame allocator can use mock frames too, similarly to M02.
    * Frame can be temporarily allocated from heap by requesting an 8 KiB
      block and returning a properly aligned address from inside the block.
    * Ensure that your address space functions work first.
    * Feel free to print the amount of available frames during kernel
      initialization so that you can quickly check your memory detection
      is correct.
    * Ensure that you build a correct page table (see the
      [`as/pagetable`](kernel/test/as/pagetable/test.c) test) without touching
      the `satp` register.
    * Use the page translation commands of MSIM to test your page table.
    * Focus first on cases in which the kernel will see only a few address
      spaces (and simply use a counter for an ASID).
    * Even though we do not prescribe an API for ASID management, we suggest to
      create one and use it to avoid interleaving the implementation details of
      ASID management with other code.
    * Consider adding functions such as `as_acquire` and `as_release` to
      capture the intent of multiple threads owning the same address space.
    * Who calls `as_destroy`?

---

## Submission

The **baseline solution** for this milestone must pass the baseline tests for
`M01`, `M02`, `M03`, `M04`, `M05` and `M06`. The commit representing the
solution must be tagged with `m07`. Submitting a **working baseline solution**
within this milestone's deadline allows you to gain bonus points towards
a better grade.

Extended features are usually covered by separate test suites (see comments on
the first line of the respective suite for details). Some features cannot be
reliably tested and will be evaluated during final review.

Please, see also the notes in the main [README.md](README.md).
