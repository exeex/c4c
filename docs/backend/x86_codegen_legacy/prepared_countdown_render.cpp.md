# `prepared_countdown_render.cpp`

Primary role: prepared-route specializer for countdown-loop and loop-join branch shapes.

Key surfaces:

```cpp
std::optional<std::string> render_prepared_loop_join_countdown_if_supported(...);
std::optional<std::string> render_prepared_countdown_entry_routes_if_supported(...);
std::optional<std::string> render_prepared_local_i32_countdown_loop_if_supported(...);
```

- Recognizes bounded countdown control-flow shapes directly from prepared control-flow metadata and renders final assembly text itself.
- Reconstructs authoritative branch targets and stack-memory operands without routing through the canonical branch owner in `comparison.cpp`.
- Depends on prepared block lookup, join-transfer facts, and local stack-layout assumptions.
- Special-case classification: `optional fast path` when treated as a thin prepared adapter; `overfit to reject` if this matcher family grows by testcase shape instead of moving ownership into canonical branch lowering.
