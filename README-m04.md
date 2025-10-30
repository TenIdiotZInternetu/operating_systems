# Milestone 04: Frame Allocator

So far, your kernel has a heap allocator (`kmalloc`) that manages a contiguous
block of memory. This works well for general allocations and the kernel's
internal data structures, but it's not sufficient for what will come later:
virtual memory. This milestone introduces a second, more specialized memory
allocator that is essential for managing memory at the hardware level.

The main goal is to implement a **physical frame allocator**. This allocator
will manage physical memory in fixed-size blocks called **frames** that are
also naturally aligned to their size, which corresponds to the page size
supported by the hardware. This is an important step towards building a virtual
memory subsystem later, which translates an application's view of memory
(_virtual pages_) into the hardware view (_physical frames_). It is also
required when working with devices which can access physical memory directly,
i.e., via Direct Memory Access (DMA), because they need to be provided with
physical memory addresses.


## A tale of two allocators

Your existing heap allocator is designed for convenience, enabling allocations
of arbitrarily-sized blocks with the granularity of bytes (and some minimal
alignment due to performance concerns and/or hardware requirements). However,
the part of the CPU responsible for mapping virtual addresses to physical
addresses (Memory Management Unit, MMU) does not work with arbitrary byte
counts; it operates on fixed-size blocks of virtual memory called pages.

This means that we need an allocator compatible with the hardware view of
memory (frames), which is what the heap allocator lacks. This creates an
interesting design challenge: your kernel needs **two allocators** managing
a **single resource**, but each with a completely different view of that
resource.

- **The heap allocator** (`kmalloc`) views memory as an array of bytes,
  allocates blocks of arbitrary sizes with minimal alignment, keeps metadata
  **inside** the memory blocks it manages (e.g., in block headers), and works
  with virtual addresses (pointers) that can be directly used by the CPU.

- **The frame allocator** views memory as an array of fixed-size blocks
  (**physical frames**), allocates blocks with the granularity and alignment
  of hardware page size (e.g., 4 KiB), keeps metadata **outside** the blocks
  it manages (e.g., in a separate bitmap of used/free frames), and works with
  **physical addresses** that represent actual locations in RAM, but cannot
  be directly used as pointers by the kernel.

The heap allocator (`kmalloc`) is unsuitable for allocating frames, because it
stores metadata within the memory blocks and its interface cannot guarantee the
strict page-size alignment required by the hardware (MMU). On the other hand,
the frame allocator can be used to allocate thread stacks, because they are
relatively large and they should eventually be page-aligned. Their size is
typically a small multiple of page size and allocating them through the frame
allocator reduces the pressure on the heap allocator.


## Frame allocator implementation

Your task is to implement the frame allocator interface defined in the
[`mm/frame.h`](kernel/include/mm/frame.h) header file. The functions are
expected to work similarly to `kmalloc()` and `kfree()`, but for page-sized
and page-aligned blocks of physical memory. Your implementation is expected
to reside in [`mm/frame.c`](kernel/src/mm/frame.c).

Because both allocators manage the same physical memory, they must cooperate
to avoid conflicts. There are two primary approaches to achieve this:

- **Static partitioning (Baseline):** This is an acceptable baseline solution.
  At boot time, your kernel can simply split the available physical memory
  into two distinct regions. One region is given exclusively to the frame
  allocator, and the other is given to the heap allocator. This is simple, but
  inflexible.
- **Layered allocators (Advanced):** This is the preferred and more advanced
  solution. The frame allocator manages _all_ usable physical memory, with the
  heap allocator (`kmalloc`) built "on top" of the frame allocator. When the
  heap allocator needs more memory to satisfy an allocation, it requests one
  or more full frames from the frame allocator to grow its own pool. This
  approach is more efficient and flexible, because the memory is allocated to
  the subsystem that needs it, when it needs it.

To track which frames are free and which are in use, you'll need a suitable
data structure, such as a bitmap. You have two options for allocating this
bookkeeping structure:

- **Statically (Baseline):** This is an accepted baseline solution. You can
  pre-allocate the bitmap at compile time. The `FRAME_COUNT_MAX` macro in the
  [`mm/frame.h`](kernel/include/mm/frame.h) header file is provided for this
  purpose. Because we assume that only 512 MiB of memory will be used, the
  actual memory consumed by a bitmap will be relatively low.
- **Dynamically (Optimal):** This the preferred and optimal solution. During
  initialization, the frame allocator should first reserve a few frames from
  the available memory to store its own bookkeeping data.

Your implementation can assume that all physical memory will fit into a 512 MiB
block starting at 2 GiB (i.e. where the kernel is loaded).
Neither your frame allocator nor your memory-probing code needs to consider
physical memory beyond this limit. The amount of available memory detected
at boot will not change.

Memory detection, as implemented in M02, will stay the same.


## Support code

This milestone updates the kernel base code only to initialize the frame
allocator, the rest of the provided skeleton remains the same. The only
substantial change is the addition of the bitmap data structure (see below).


### Bitmaps

For your convenience, we provide a simple bitmap implementation. The interface
can be found in the [`adt/bitmap.h`](kernel/include/adt/bitmap.h) header file.
You are welcome to use it, but it is not required. However, if you decide to
use your own data structure, make sure that its implementation details are not
intermixed with the frame allocator code.


## Tests

New tests are provided for this milestone that will check the functionality
of the frame allocator. However, do not forget to run also the tests for the
heap allocator to ensure that your frame allocator and heap allocator can
happily coexist.


## Additional hints/suggestions

* Allocate thread stacks using the frame allocator. You will need a much
  smaller heap then. Think about whether you can use the results of
  `frame_alloc()` directly, or whether you should use an extra layer of
  abstraction. e.g., a function such as `stack_alloc()`.
* Clearly mark variables that hold physical addresses so you do not
  accidentally treat them as regular pointers. Never store them as `void *`.
* For initial development and testing, you can implement a temporary frame
  allocator that gets its "frames" from the heap by requesting a larger
  block (e.g., 8 KiB) and returning a properly aligned address from within
  the block. You can also use mock frames, similarly to M02.
* Feel free to print the total number of available frames during kernel
  initialization to quickly check that your memory detection is correct.

---

## Submission

The **baseline solution** for this milestone must pass the baseline tests for
`M01`, `M02`, `M03` and `M04`. The commit representing the solution must be
tagged with `m04`. Submitting a **working baseline solution** within this
milestone's deadline allows you to gain bonus points towards a better grade.

Extended features are usually covered by separate test suites (see comments on
the first line of the respective suite for details). Some features cannot be
reliably tested and will be evaluated during final review.

Please, see also the notes in the main [README.md](README.md).
