# `route_debug.cpp`

Primary role: proof/debug surface for summarizing and tracing prepared-route matches.

Key surfaces:

```cpp
std::string summarize_prepared_module_routes(const c4c::backend::prepare::PreparedBirModule&, std::optional<std::string_view>, std::optional<std::string_view>);
std::string trace_prepared_module_routes(const c4c::backend::prepare::PreparedBirModule&, std::optional<std::string_view>, std::optional<std::string_view>);
std::string render_single_block_void_call_sequence_facts(...);
```

- Converts prepared-route facts into human-readable summaries and traces rather than emitting machine code.
- Mirrors many narrow prepared matcher families as diagnostic vocabulary, which makes it a useful extraction/proof artifact.
- Depends on the same prepared control-flow and value-shape facts used by specialized renderers.
- Special-case classification: `legacy compatibility` for narrow matcher introspection; this is a proof surface, not a lowering owner.
