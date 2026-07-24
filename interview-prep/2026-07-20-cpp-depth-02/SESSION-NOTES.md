# Session 02 — C++ Depth

- **Date:** 2026-07-20
- **Format:** Step-by-step interactive (new default: poker-faced interviewer, checkpoints, occasional bluffs)
- **Category:** C++ depth — reference binding, lifetime extension, vector reallocation, `move_if_noexcept`, pointer invalidation
- **Calibration:** Lean hire — but with a *different and better* failure profile than session 01 (see verdict)

---

## The Question

Trace the full output (C++17, capacity grows 0→1→2):

```cpp
#include <cstdio>
#include <vector>

struct Tracker {
    int id;
    Tracker(int i) : id(i)              { printf("ctor %d\n", id); }
    ~Tracker()                          { printf("dtor %d\n", id); }
    Tracker(const Tracker& o) : id(o.id) { printf("copy %d\n", id); }
    Tracker(Tracker&& o) noexcept : id(o.id) {
        o.id = -o.id;                   // mark source as moved-from
        printf("move %d\n", id);
    }
};

Tracker make(int i) { return Tracker(i); }

int main() {
    const Tracker& r = make(1);      // (A)
    std::vector<Tracker> v;
    v.push_back(make(2));            // (B)
    Tracker* p = &v[0];
    v.push_back(make(3));            // (C)
    printf("p->id: %d\n", p->id);    // (D) UB — dangling
    printf("r.id:  %d\n", r.id);     // (E)
    return 0;
}                                     // (F)
```

**Full answer:**

```
ctor 1            // (A) guaranteed elision into the lifetime-extended temporary
ctor 2            // (B) temporary
move 2            //     into vector
dtor -2           //     temporary dies at end of full expression
ctor 3            // (C) temporary
move 3            //     new element into NEW buffer (order vs next line = impl detail)
move 2            //     old element relocated (MOVE because move ctor is noexcept)
dtor -2           //     old element destroyed in old buffer
dtor -3           //     temporary dies
p->id: <UB>       // (D) dangling after reallocation; prints -2 in practice
r.id:  1          // (E) lifetime extension held
dtor 2            // (F) vector elements: FORWARD order (impl), not reverse
dtor 3
dtor 1            //     r's extended temporary, last (reverse scope order)
```

---

## Knowledge Gaps Exposed (learn these cold — both are canonical senior filters)

### 1. The reference binding table
Stated "references only bind to lvalues" → wrong compile-error prediction at (A).
The complete rule, three lines:

- non-const lvalue ref (`T&`) → lvalues only
- **const lvalue ref (`const T&`) → lvalues AND rvalues** ← the everyday one
- rvalue ref (`T&&`) → rvalues only

**Temporary lifetime extension:** binding a temporary *directly* to a const ref
at local scope extends its lifetime to the reference's scope. It stays an
ordinary automatic object (NOT static — I invented a wrong "static storage"
mechanism from string-literal intuition). Extension does NOT survive
indirection: `const T& r = f();` where `f` returns a reference to a temporary
→ dangles. Only direct binding counts.

### 2. `move_if_noexcept` — the noexcept/vector contract
Guessed memcpy first (C-brain), then honestly didn't know move vs copy.
The chain to own:

1. Reallocation wants the **strong exception guarantee** (throw ⇒ vector unchanged).
2. Move-relocation can't roll back: sources already gutted when element k+1 throws.
3. Copy-relocation can: old buffer pristine, free new block, rethrow.
4. So vector moves **only if the move ctor is `noexcept`** (or type is move-only):
   `std::move_if_noexcept`.

**Production war story to tell interviewers:** delete one `noexcept` from a move
ctor and every vector growth silently switches to copying — a real, common
profiling find at game studios. This is MY domain; I should be telling this
story, not learning it.

### 3. Smaller corrections
- `memcpy` relocation requires **trivially copyable** — any user-provided
  copy/move ctor or dtor kills that. A move ctor with side effects (`-id`)
  obviously can't be memcpy'd.
- `std::vector<T&>` is ill-formed because references aren't objects (no
  addresses, not assignable) — fails element requirements at compile time,
  not "dangling at realloc". Use `std::reference_wrapper` if genuinely needed.
- Container element destruction order is **unspecified**; implementations
  destroy **forward**. "Reverse order" applies to scope objects only.
- Reading through a dangling pointer often prints plausible garbage (the
  moved-from corpse value) — that's what makes it worse than a clean crash.

## What Went Right

- **The session-01 retest passed.** Tracked moved-from markers (`-2`, `-3`)
  correctly through the entire trace, unprompted, including the subtle
  "internally consistent" detail: my (wrong) copy-based list printed `dtor 2`
  not `dtor -2` — right bookkeeping on a wrong premise. The state-tracking
  slippage from session 01 did not recur.
- Dangling-pointer invalidation at (C)→(D) caught unprompted, UB correctly named.
- Guaranteed elision at (A), overload resolution to `push_back(T&&)`,
  size/capacity reasoning, geometric growth — all clean.
- Recovered (B)'s missing `dtor -2` with a single nudge.
- Said "I don't know" cleanly when out of road on move-vs-copy. Correct
  interview behavior; interviewers score calibration, not just knowledge.

## Verdict / Trend

Session 01's failure was a **pressure slip** (knew the fact, lost the state).
Session 02's failures were **knowledge gaps** (binding rule, move_if_noexcept)
— while the state-tracking, the thing we're actually training, held up well
under an interactive, bluff-capable format. That's the right direction:
knowledge gaps are fixed by reading; pressure slips only by reps.

Watch for: when unsure, I reached for C-mental-models (memcpy, static strings)
instead of C++ object semantics. Under pressure I regress to C-land. Next
sessions should keep testing whether that reflex resurfaces.

## Next Time I See This Pattern

Trigger phrases: *const ref to temporary*, *push_back/reallocation*,
*noexcept on move ctor*, *pointer/iterator invalidation*, *what does this print*.

Checklist:
1. `const T&` binds rvalues; direct binding extends lifetime to ref's scope.
2. Any user-provided copy/move/dtor ⇒ NOT trivially copyable ⇒ no memcpy;
   every object's death prints/runs a dtor — silent disappearance = wrong trace.
3. Vector growth: `noexcept` move ⇒ moves; otherwise copies. Say the rule name:
   `move_if_noexcept`, strong exception guarantee.
4. Reallocation invalidates ALL pointers/references/iterators into the vector.
5. Scope objects die in reverse; container elements in unspecified
   (practically forward) order.
