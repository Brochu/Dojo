# Session 03 — C++ Depth

- **Date:** 2026-07-20
- **Format:** Step-by-step interactive, poker-faced interviewer
- **Category:** C++ depth — slicing, virtual dispatch machinery (vptr/vtable), delete-through-base UB
- **Calibration:** Hire — strongest session yet; gaps were in follow-up machinery, not the core trace

---

## The Question

Trace the output (C++17):

```cpp
#include <cstdio>

struct Base {
    Base()                  { printf("B ctor\n"); }
    virtual void id() const { printf("I am Base\n"); }
    ~Base()                 { printf("B dtor\n"); }     // (!) not virtual
};

struct Derived : Base {
    int* payload;
    Derived() : payload(new int[16]) { printf("D ctor\n"); }
    void id() const override         { printf("I am Derived\n"); }
    ~Derived()                       { delete[] payload; printf("D dtor\n"); }
};

void greet(Base b) { b.id(); }       // (A) by VALUE — slicing

int main() {
    Derived d;                       // (1)
    greet(d);                        // (A)
    Base* p = new Derived();         // (B)
    p->id();                         // (C)
    delete p;                        // (D) UB — non-virtual dtor
    return 0;
}                                    // (E)
```

**Full output (I got this entirely right):**

```
B ctor          // (1) base subobject first
D ctor
I am Base       // (A) sliced copy via IMPLICIT copy ctor (no print!) → static Base
B dtor          //     the copy dies at greet's exit — dtor with no ctor print
B ctor          // (B)
D ctor
I am Derived    // (C) virtual dispatch through pointer
B dtor          // (D) UB: only ~Base runs in practice; payload LEAKS
D dtor          // (E) d: derived-then-base, reverse of construction
B dtor
```

---

## What I Got Right (unprompted)

- The slicing trap cold: implicit copy ctor (silent), sliced copy is a real
  `Base`, static dispatch, and the unmatched-looking `B dtor` — the ledger
  discipline from sessions 01–02 is now a reflex.
- Construction/destruction ordering (base→derived; reverse on destroy).
- (D) caught immediately: non-virtual dtor, only `B dtor`, leak, fix = virtual.
- Full-program ledger, no errors.

## Knowledge Gaps Exposed

### 1. Dispatch-through-what rule (missed, then corrected)
Initially said const-ref parameter still prints "I am Base" (reasoned about
lifetime, forgot dispatch). The rule:

> **Dispatch is virtual through pointers and references; static through
> values.** "Copy the object, lose the type." Slicing doesn't disable the
> vtable — the sliced copy genuinely IS a Base.

### 2. The vptr (didn't know the object-side half of the mechanism)
- Per-CLASS vtable in read-only data; per-OBJECT hidden **vptr** written by
  the ctor. `sizeof` proof: `{int}` = 4; `{int + any virtual}` = 16
  (8-byte vptr + int + padding).
- Call sequence for `p->id()`: load vptr from object → load fn address from
  vtable slot → indirect call. **Two dependent loads** (pointer chase).
- Vtables are flattened at compile time — overridden slots are REPLACED.
  Dispatch is O(1); no inheritance-tree walking ever (I guessed that; wrong).

### 3. True cost of an indirect call (undersold as "an extra indirection")
The perf-engineer answer, in order of pain:
1. **No inlining** — call is opaque to the optimizer; kills constant
   propagation, vectorization, DCE across the boundary. The gateway
   optimization is gone. Usually THE dominant cost.
2. **Indirect branch prediction** — target must be predicted (BTB);
   megamorphic call sites (heterogeneous `vector<Entity*>` update loop)
   mispredict → pipeline flush ~15–20 cycles.
3. Two dependent loads, each a potential cache miss.
4. Ambient: +8 bytes per object (cache density — the formal version of the
   DOD instinct), scattered call targets → icache pressure.
- Bonus: **devirtualization** — `final` is a performance tool; compilers
  devirtualize when dynamic type is provable (final/LTO/PGO).

### 4. UB ladder (had it right, talked myself OUT of it)
Wrote "UB" first, then reasoned "I can predict it, so it feels defined."
The four rungs: **well-defined / implementation-defined (documented choice) /
unspecified (valid set, undocumented) / undefined (NO requirements)**.
Predictability on today's compiler is an accident, not a promise — the
optimizer may assume the UB never happens and transform accordingly.

### 5. Sutter's dtor guideline (said "private"; it's "protected")
> Base class destructors: **public and virtual** (polymorphic deletion
> supported) OR **protected and non-virtual** (inheritable, but outsiders
> can't delete through Base* — the bug becomes a compile error).
Private non-virtual fails because every derived dtor implicitly calls the
base dtor — private makes the class underivable in practice.

## Pattern Watch

- **New failure mode observed: second-guessing a correct first instinct**
  (the UB call). Different from session 01's state-slippage — this is
  calibration under pressure. First instinct + a reason to doubt it ≠
  abandon it; demand a mechanism before overturning yourself.
- C-land regression reflex (sessions 02) did NOT appear this time.
- Honesty streak continued (three clean "I don't know"s on vptr machinery) —
  kept the session moving, scored as senior behavior.

## Next Time I See This Pattern

Trigger phrases: *pass polymorphic type by value*, *delete through base
pointer*, *sizeof a class with virtuals*, *cost of virtual calls*.

Checklist:
1. Parameter by value + polymorphic argument = slicing; implicit copy prints
   nothing but the copy still DIES (dtor with no ctor print is not an error).
2. Virtual through `*` and `&`; static through values.
3. `new Derived` into `Base*` → check the dtor for `virtual` IMMEDIATELY.
4. Mechanism on demand: vptr → vtable slot → indirect call; costs: inlining
   loss > branch misprediction > dependent loads > object bloat.
5. Never demote UB because the observed behavior looks stable.
