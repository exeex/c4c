# `prepared_local_slot_render.cpp`

Primary role: the largest prepared-route renderer, mixing local-slot layout, prepared addressing, byval/HFA handling, call-lane helpers, and many shape-specific render paths.

Key surfaces:

```cpp
std::optional<PreparedModuleLocalSlotLayout> build_prepared_module_local_slot_layout(...);
std::optional<std::string> render_prepared_symbol_memory_operand_if_supported(...);
std::optional<std::size_t> find_prepared_value_home_frame_offset(...);
```

- Builds a prepared-local-slot/frame model from stack-layout, addressing, prepared names, and value-home metadata before rendering machine operands.
- Reimplements large parts of canonical memory, call, and local-layout ownership inside renderer-local helpers.
- Acts as the clearest evidence that the prepared family has become a parallel lowering stack rather than a thin client of `memory.cpp`, `calls.cpp`, and `prologue.cpp`.
- Special-case classification: mixed `legacy compatibility` and `optional fast path`; this file is the strongest current candidate for `overfit to reject` growth if more narrow route matching lands here.
