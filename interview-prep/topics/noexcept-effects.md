# noexcept — Everywhere It Changes Behavior

(Follow-up to session 02's `move_if_noexcept` discussion.)

Mental model: `noexcept` is not an annotation, it's a **trait the ecosystem
branches on** — optimizer, library, type system, runtime.

## Runtime semantics
- Exception escaping a `noexcept` function → `std::terminate`. Stack
  unwinding before terminating is *implementation-defined* — RAII cleanup may
  be skipped entirely.
- Compiler may drop exception paths/unwind handling around noexcept calls →
  smaller code, modest direct perf win.
- **Destructors are implicitly noexcept** since C++11. Throwing dtor
  terminates unless `noexcept(false)` — and still terminates during unwind.

## Compiler-generated members compute it
- Defaulted move ctor/assign are noexcept **iff every member + base is**.
- Trap: add one throwing-move member → whole class loses nothrow-move →
  every `vector<T>` silently switches to copy-relocation. No diagnostic.
- Defense (senior signal): `static_assert(std::is_nothrow_move_constructible_v<T>)`
  on hot types.

## Library dispatch (the move_if_noexcept family)
- All growing/shifting vector ops (`push_back`, `emplace_back`, `insert`,
  `resize`, `reserve`): move-relocate only if nothrow move; else copy.
  Reason: strong exception guarantee — mid-relocation move throw is
  unrecoverable, copy throw is rollback-able.
- `std::variant`: throwing move mid-assignment → `valueless_by_exception`.
  Nothrow moves make that state unreachable.
- `std::any` (and `std::function` in practice): small-buffer optimization
  gated on nothrow move — non-noexcept move ⇒ small type heap-allocates.
- `std::swap`: conditional noexcept propagated from the type's moves;
  copy-and-swap is only "strong" if swap can't throw.

## Type system (C++17: noexcept is part of the function TYPE)
- `void(*)() noexcept` ≠ `void(*)()`. Throwing → noexcept pointer: compile
  error. Noexcept → throwing pointer: fine.
- Virtual override of a `noexcept` virtual must itself be noexcept — enforced
  at compile time.
- `noexcept(expr)` operator: compile-time "would this throw?" query. Enables
  conditional specs: `noexcept(noexcept(f()))` — "nothrow if f is."
  (Interviewers ask why it's written twice: outer = specifier, inner = operator.)

## One-line interview answer
"It's a contract the optimizer, the library, and the type system all consume —
and the most expensive place it shows up is silently, when it's missing."
