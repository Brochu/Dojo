# Session 01 — C++ Depth (Practice Run)

- **Date:** 2026-07-18 (evening, cold start)
- **Format:** Verbal mock, single question, interviewer pushback style
- **Category:** C++ depth — object lifetime, move semantics, rule of five
- **Calibration:** Lean hire (senior C++ bar) — solid fundamentals, one error self-corrected after prompting

---

## The Question

"Walk me through what this prints, and flag anything you'd push back on in review." (C++17, modern compiler)

```cpp
#include <cstdio>
#include <cstring>

struct Buffer {
    char*  data;
    size_t size;

    Buffer(size_t n) : data(new char[n]), size(n) { printf("ctor\n"); }
    ~Buffer() { printf("dtor\n"); delete[] data; }

    Buffer(const Buffer& other) : data(new char[other.size]), size(other.size) {
        printf("copy\n");
        memcpy(data, other.data, size);
    }

    Buffer(Buffer&& other) : data(other.data), size(other.size) {
        printf("move\n");
        other.data = nullptr;
        other.size = 0;
    }
};

Buffer make_buffer() {
    Buffer b(64);
    return b;
}

int main() {
    Buffer a = make_buffer();
    Buffer b = std::move(a);
    Buffer c = a;              // <-- the trap: a is moved-from here
    printf("size: %zu\n", c.size);
    return 0;
}
```

**Actual output:** `ctor`, `move`, `copy`, `size: 0`, then 3x `dtor`. No crash.

---

## What I Got Right (unprompted)

- RVO in `make_buffer` — single "ctor", no temporary cleanup
- Move mechanics — ownership transfer, `a` nulled out, no leak
- Destruction in reverse declaration order (c, b, a)
- `delete[] nullptr` is a safe no-op
- Flagged raw-pointer ownership readability; suggested smart pointers
- Named rule of 3 / rule of 5 as the governing principle

## The Error (the one to internalize)

**Copied from `a` after tracing the move that invalidated it.** I said out loud
"the move sets `a.data` to nullptr" — then two lines later treated `a` as valid
and answered `size: 64`. I was mentally copying from `b`.

**Root cause:** not a knowledge gap — *state-tracking slippage while narrating*.
My mouth was on line 3 while my mental machine state was stale. This is the
under-pressure failure mode I'm here to train out.

**The fix (mechanical, always applies):** for any "what does this print"
question, annotate each variable's state per line before answering — even just
mentally: `a: {null, 0}`. Ten seconds slower, immune to this entire error class.

## Second Miss (severity ordering)

On `b = a;` with the compiler-generated copy assignment (shallow copy), I found
the **leak** of `b`'s old allocation but had to be pushed to find the **double
free** — both dtors deleting the same pointer. In review, always name the worst
bug first: crash-at-the-scene < leak < corruption-at-a-distance.

---

## Knowledge Nuggets Picked Up

1. **`memcpy(dst, nullptr, 0)` is UB** per the standard even with count 0 —
   works in practice, but the optimizer may assume the pointer is non-null
   afterwards and delete downstream null checks. (C++26 finally made
   null + zero-length well-defined.)
2. **`new char[0]` is legal** — returns a valid, non-dereferenceable pointer
   that must still be `delete[]`'d.
3. **Double free corrupts allocator bookkeeping, not your data** — the crash
   comes later, in unrelated code, on a possibly different thread, with an
   innocent-looking callstack. That's *why* it's the nastiest category here.
4. **Implicitly-generated copy assignment still exists** when you've written
   your own copy/move ctors and dtor (deprecated, but generated) — and it's a
   shallow copy. Move assignment is NOT generated in that case.

## Language Precision Notes

- Don't say "null ref exception" (C#/Java-ism). Say **"access violation"** or
  **"segfault"** — word choice reads as rust in a C++ room.

## Interview Meta-Lessons

- When the interviewer says "walk me through the state at this exact line,"
  **they're telling you where the body is buried.** Free hint — slow down there.
- Recovery-under-pushback is heavily weighted, and it went well both times:
  fast, clean re-trace, no defensiveness. Keep that.
- What separates lean-hire from strong-hire on this question: catching the
  moved-from copy **unprompted**.

---

## Next Time I See This Pattern

Trigger phrases: *moved-from object*, *use after move*, *rule of five*,
*compiler-generated assignment*, *shallow copy*.

Checklist:
1. Per-line state annotation of every owning object (`ptr`, `size`).
2. After any `std::move(x)`, mark `x` as radioactive — every later use of `x`
   is a suspect.
3. Class owns raw memory? Immediately audit all five: dtor, copy ctor, move
   ctor, copy assign, move assign. Missing ones = shallow copy = double free.
4. Report bugs worst-first: corruption > crash > leak > style.
