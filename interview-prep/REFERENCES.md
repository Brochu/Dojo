# References — C++ Interview Prep

Strategy: read *reactively* — sessions find the gaps, then do targeted reading
with the failure fresh. Don't grind references cover-to-cover.

## Daily lookup
- **cppreference.com** — the canonical practicing-dev reference. Bookmarks:
  - [vector::push_back](https://en.cppreference.com/w/cpp/container/vector/push_back) — invalidation + move-vs-copy on reallocation (session 02)
  - [std::move_if_noexcept](https://en.cppreference.com/w/cpp/utility/move_if_noexcept) (session 02)
  - [Reference initialization](https://en.cppreference.com/w/cpp/language/reference_initialization) — const-ref binding + temporary lifetime extension (session 02)
  - [Value categories](https://en.cppreference.com/w/cpp/language/value_category)

## Books
- **Effective Modern C++** — Scott Meyers. The priority read: ~40 items, each
  basically a senior interview question. Item 14 = noexcept + vector realloc.
  Covers move semantics, value categories, forwarding refs, lambda pitfalls.
- **C++ Move Semantics – The Complete Book** — Nicolai Josuttis. Optional
  deep-dive if Meyers leaves questions on value categories / moved-from state.
- (Later, for category 5 sessions: **C++ Concurrency in Action** — Anthony
  Williams. The book on atomics/memory ordering.)

## Reps between sessions
- **cppquiz.org** — "what does this print" puzzles with explanations citing
  the standard. Same format as our drills; good for the per-line
  state-annotation habit. Five-minute doses.

## The standard itself
- **eel.is/c++draft** — free hyperlinked working draft. For settling disputes
  only; cppreference paraphrases it better for study.

## Video
- CppCon **"Back to Basics"** track (YouTube) — Klaus Iglberger, *Back to
  Basics: Move Semantics* parts 1 & 2 ≈ session 02's entire territory, aimed
  at experienced devs.
