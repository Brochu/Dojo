# Progress Ledger

Rolling state for interview prep. **Read this instead of the full session
archive.** Update at the end of every session; keep it under ~100 lines by
retiring resolved items.

- **Target bar:** senior graphics programmer / GPU hardware vendor
- **Sessions completed:** 4 (next folder number: `05`)
- **Last session:** 2026-07-20 — concurrency (memory ordering, release/acquire)

## Calibration trend

| # | Date | Category | Grade |
|---|------|----------|-------|
| 01 | 2026-07-18 | C++ depth — lifetime, move semantics, rule of five | Lean hire |
| 02 | 2026-07-20 | C++ depth — reference binding, `move_if_noexcept`, invalidation | Lean hire |
| 03 | 2026-07-20 | C++ depth — slicing, vptr/vtable, delete-through-base UB | Hire |
| 04 | 2026-07-20 | Concurrency — memory ordering, data races | Hire (lean hire for a lock-free specialist role) |

## Standing failure patterns

1. **Talks himself out of a correct instinct under follow-up pressure.**
   Session 03 (UB ladder — had it right, abandoned it). Did **not** recur in
   session 04. Currently the primary pressure-slip signature; keep probing.
2. **Strong operational intuition, missing standard vocabulary.** Can fix the
   code and describe the mechanism but can't name the term of art. Reads as
   "has used this" rather than "understands the model." Session 04 was the
   clearest instance (couldn't produce *synchronizes-with* /
   *happens-before*); session 03 echoed it (*protected*, not *private*, dtor).
3. **Regresses to C mental models when unsure** — falls back to
   `memcpy`/static-storage reasoning instead of the language model. (S02)
4. **State-tracking slippage while narrating** — loses program state he has
   already correctly traced. (S01; held in S02–S04, but re-verify on any long
   multi-step trace.)
5. **Undersells hardware costs he actually knows** — e.g. indirect call
   described as "an extra indirection." Real strength stated too weakly.

## Retest queue

Draw at least one per session, disguised and unflagged.

- [ ] **Data-race definition, stated cold** — two conflicting accesses, ≥1 a
      write, not both atomic, not ordered by happens-before. Must be produced
      unprompted, in those terms. (S04)
- [ ] **Two sources of reordering** — compiler *and* CPU; on x86 TSO suspect
      the compiler first. (S04)
- [ ] **Reference binding table** — which references bind to which value
      categories, and lifetime extension rules. (S02)
- [ ] **`move_if_noexcept` / the noexcept–vector contract.** (S02)
- [ ] **UB ladder under pushback** — the specific test: state it correctly,
      then get pushed, and see whether it survives. (S03, pattern #1)
- [ ] **Dispatch-through-what rule** and the object-side half (vptr). (S03)
- [ ] **Severity ordering** of multiple defects in one program — lead with the
      worst. (S01)
- [ ] **C-mental-model reflex** — construct a question where the `memcpy` /
      static-storage answer is plausible but wrong. (S02, pattern #3)
- [ ] **Long multi-step trace under interruption** — verify state-tracking
      still holds when interrupted mid-narration. (S01, pattern #4)

## Covered ground — don't repeat verbatim

Object lifetime, rule of five, move semantics, reference binding, lifetime
extension, vector reallocation/pointer invalidation, `move_if_noexcept`,
slicing, virtual dispatch machinery, delete-through-base UB, `noexcept`
effects (see `topics/noexcept-effects.md`), memory ordering, release/acquire,
data races, x86 TSO cost model.

**Categories not yet touched:** 1 (coding/algorithms), 3 (low-level/perf),
4 (GPU/graphics), 6 (system design), 7 (OOD), 8 (behavioral), 9 (process),
10 (live debugging). Given the target bar, 4 and 3 are the most conspicuous
gaps in the record — no GPU or performance session has been run yet.
