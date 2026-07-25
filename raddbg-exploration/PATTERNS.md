# RAD Debugger Codebase Patterns & Idioms

## Codebase Map

RAD Debugger is organized as layered, specialized libraries:

- src/base/ — Foundation types, memory allocators, string handling, threading.
- src/os/ — Operating system abstraction layer.
- src/metagen/ — Code generation tool.
- src/ui/ — Immediate-mode UI framework.
- src/dbg_engine/ — Debugger logic.
- src/dwarf/, src/codeview/, src/elf/ — Debug info parsers.

Most targets use Unity builds.

---

## 18 Distinctive Patterns

### 1. Arena Allocators with Chaining & Temp Scopes
src/base/base_arena.h:1-88, src/base/base_arena.c:84-190
High-performance allocator. Allocate freely, pop back to checkpoint. Zero-cost destructor.

### 2. String8 Slices Without Ownership
src/base/base_strings.h:10-15, src/base/base_strings.h:208-214
No allocation, no copies. Slicing is O(1).

### 3. Intrusive Linked Lists via Macros
src/base/base_core.h:307-346
Zero allocation overhead. Macros handle nil-pointer edge cases.

### 4. Type System: U64, S32, B32
src/base/base_core.h:442-485
Short names reduce clutter. Consistent with game-engine style.

### 5. Operator Vocabulary: Push/Pop, Make/Release, Open/Close
src/base/base_arena.h:65-76
Disciplines mental model. All open/close and push/pop are balanced.

### 6. Code Generation via Metagen & .mdesk DSL
src/metagen/metagen.h:1-100, src/ui/ui.mdesk:1-94
One source of truth for field defaults. Avoids hand-rolling repetitive types.

### 7. Thread-Local Scratch Arenas & Per-Thread Context
src/base/base_thread_context.h:54-80
No arena parameter in every function. Two-arena trick avoids nesting conflicts.

### 8. No Exceptions, Return Codes, or Out-Parameters
src/base/base_core.h:238-243
Simplifies API. No exception chains. Parsing errors handled via structured returns.

### 9. Memory Operation Macros for Bulk Copy/Zero
src/base/base_core.h:201-225
Prevents size/type mismatches. Self-documenting.

### 10. For-Each Loop Macros
src/base/base_core.h:193-198
Exposes iteration invariants clearly. Avoids off-by-one bugs.

### 11. DeferLoop for Resource Cleanup
src/base/base_core.h:190-191
Structured cleanup without C++ destructors. Pairs setup/teardown lexically.

### 12. Naming Conventions: Module Prefixes and Enum Qualification
src/base/base_arena.h:65-76, src/ui/ui_core.h:10-32
No namespace pollution. IDE autocomplete powerful. Enum values unambiguous.

### 13. Atomic Operations via Cross-Platform Macros
src/base/base_core.h:250-297
Lock-free patterns work same on all platforms. Abstraction thin enough for inlining.

### 14. Hash Table with Chaining and Free Lists
src/linker/hash_table.h:35-41, src/linker/hash_table.h:50-86
Integrates with arena. Free list reuses nodes.

### 15. Immediate-Mode UI with Stack-Based State
src/ui/ui_core.h:1-160, src/ui/ui.mdesk:6-85
No parameter bloat. Stacks mirror C scope. Codegen avoids repetition.

### 16. Linked-List Node Pairs for String8List
src/base/base_strings.h:34-48, src/base/base_strings.h:260-270
O(1) push, O(n) join. Single allocation beats repeated str8_cat.

### 17. Safe Casts with Assertions
src/base/base_core.h:919-923, src/base/base_core.c:5-27
Documents intent. Zero overhead in release.

### 18. Global Bitmasks and Bit Helpers
src/base/base_core.h:710-840, src/base/base_core.h:423-426
Compile-time constants compile away to immediates. No runtime computation.

---

## Further Reading

1. src/base/base_core.h — Type system, memory macros, linked-list.
2. src/base/base_arena.h & base_arena.c — Arena allocation deeply.
3. src/base/base_strings.h & base_strings.c — String slicing; zero-copy.
4. src/base/base_thread_context.h — Per-thread state; scratch arenas.
5. src/ui/ui.mdesk & src/ui/ui_core.h — Metagen in action; stack-based UI.
6. src/linker/hash_table.h & hash_table.c — Typed hash table design.
7. src/metagen/metagen.h — How code generator works.
8. src/raddbg/raddbg.c — How all layers compose.

---

## Summary

RAD Debugger is a masterclass in memory safety without GC, code clarity via naming, and metaprogramming for boilerplate elimination.

Key takeaways:
- Arena allocation is the foundation.
- String slices (borrowed, not owned) simplify APIs.
- Intrusive lists and macros solve generic problems without runtime cost.
- Metagen eliminates boilerplate without loss of clarity.
- Naming discipline (module prefixes, enum qualification, verb-based APIs) multiplies readability.
- No exceptions: structured error handling scales well.
