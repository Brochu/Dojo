# Session 04 — Concurrency (surprise category)

- **Date:** 2026-07-20
- **Format:** Step-by-step interactive, poker-faced interviewer, category NOT announced in advance
- **Category:** 5 — Concurrency: memory ordering, data races, release/acquire
- **Calibration:** Hire for senior generalist. Lean hire for a lock-free/concurrency-specialist role
  (operational understanding solid; standard vocabulary was missing).

---

## The Question

```cpp
int              data  = 0;                          // plain, non-atomic
std::atomic<bool> ready{false};

void producer() {
    data = 42;                                        // (1)
    ready.store(true, std::memory_order_relaxed);     // (2)
}
void consumer() {
    while (!ready.load(std::memory_order_relaxed)) {} // (3)
    printf("%d\n", data);                             // (4)
}
```

**Verdict:** broken. `relaxed` creates no synchronizes-with edge, so the write
at (1) does not happen-before the read at (4) → **data race → undefined
behavior**. Printing 0 is just the polite failure; the standard permits
anything.

**Fix:** `release` on the store at (2), `acquire` on the load at (3).

---

## What I Got Right

- Correct diagnosis: not correct, can print 0; relaxed gives no cross-thread
  ordering guarantee.
- Named **UB** for the non-atomic concurrent access (carried over from
  session 03's UB ladder — the concept transferred cold into a new domain).
- Correct fix, unprompted: release/acquire on exactly the right operations.
- **One-way barrier semantics, correctly stated:** release = nothing before it
  sinks below; acquire = nothing after it is hoisted above. (Widen slightly:
  applies to all memory operations, not just writes/reads respectively.)
- **x86 cost model — both halves right, including the part most candidates
  miss:** acquire/release are FREE on x86-64 (plain `mov`, identical codegen
  to relaxed, because TSO already orders load-load and store-store in
  hardware; the orders act purely as *compiler* barriers). `seq_cst` puts its
  cost on the **store** side — `xchg` (implicitly locked) or `mov`+`mfence`,
  ~20–50 cycles + store-buffer drain — while the seq_cst **load** stays a
  plain `mov`.

## Knowledge Gaps Exposed

### 1. Reordering has TWO sources — I only named the CPU
- **Compiler** reordering (at compile time, under the as-if rule) AND **CPU**
  reordering (out-of-order execution, store buffers).
- The bait I fell for: the problem said **x86-64**, which is **TSO (Total
  Store Order)** — hardware does NOT reorder store-store or load-load. So on
  x86 this bug is *purely a compiler bug*. It would appear to "work" in
  testing and detonate on ARM, on a new compiler version, or under LTO.
- Corollary: a memory-order annotation must fence *both* sources.
- Also: my "stale cache / must invalidate to read from memory" mental model is
  **folklore**. x86 caches are coherent in hardware (MESI) — a retired store
  is globally visible. Memory ordering is about **reordering**, not staleness.

### 2. Vocabulary — didn't know the terms of art (guessed "dependent")
- **synchronizes-with**: a release store synchronizes-with an acquire load
  that reads the value it wrote. The inter-thread bridge.
- **happens-before**: the partial order that determines visibility. A
  synchronizes-with edge establishes it.

### 3. The crux: why does synchronizing on `ready` protect `data`?
My answer was circular ("the guarantees mean there's no race"). The actual
resolution is the **definition** of a data race:

> Two conflicting accesses to the same memory location — at least one a write,
> not both atomic — **not ordered by happens-before**.

The release/acquire edge on `ready` orders everything sequenced before the
release ahead of everything sequenced after the acquire — *including the
plain write to `data`*. So the accesses ARE ordered by happens-before and the
definition **no longer applies**. You don't "prevent" the race; you make the
term inapplicable.

**The piggyback model:** the atomic is a *vehicle*. All non-atomic payload
written before the release rides across the edge. That's why a single flag
can safely publish a megabyte struct — the core idea behind every SPSC queue
and lock-free ring buffer.

## Pattern Watch

- The session-03 failure (talking myself out of a correct instinct) did NOT
  recur.
- New observation: **strong operational intuition, missing standard
  vocabulary.** I could fix the code and describe the barriers, but couldn't
  name synchronizes-with / happens-before or state the data-race definition.
  In an interview that reads as "has used atomics" rather than "understands
  the memory model." Cheap to fix — it's vocabulary, not understanding.
- Hardware knowledge (TSO, fences, cost model) is a genuine strength; lean on
  it, but pair it with the *language-level* model, since the compiler is the
  reorderer the hardware intuition can't see.

## Next Time I See This Pattern

Trigger phrases: *flag + payload*, *relaxed*, *lock-free queue*, *publish /
handshake*, *"is this thread-safe?"*

Checklist:
1. Non-atomic object touched by 2+ threads, one writing → data race → UB.
   Say it in that order; lead with UB, not "might see a stale value."
2. Ask: is there a **happens-before** edge? Only release/acquire (or seq_cst,
   or a mutex) creates one. `relaxed` never does.
3. Name the reorderer: **compiler AND CPU**. On x86 (TSO) suspect the
   compiler first — "works on my machine" is the signature of this bug.
4. Publish pattern = release store; consume = acquire load; payload rides
   along non-atomically.
5. Costs (x86-64): relaxed == acquire/release == free (`mov`);
   seq_cst store == `xchg`/`mfence` (expensive), seq_cst load == free.
