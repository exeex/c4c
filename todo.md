Status: Active
Source Idea Path: ideas/open/261_aarch64_f128_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Retire The F128 Markdown Shard

# Current Packet

## Just Finished

Step 5: Retire The F128 Markdown Shard reconciled the legacy
`src/backend/mir/aarch64/codegen/f128.md` notes against the compiled f128
owners and deleted the markdown shard. Durable live behavior is represented in
`f128.cpp`/`f128.hpp` by prepared f128 carrier transport records, 16-byte
size/alignment checks, q-register and scalar bridge spelling, runtime helper
boundary records for arithmetic/comparison/F32/F64 casts, clobber/resource/live
preservation validation, selected-call ownership checks, and target-unsupported
printer diagnostics for incomplete facts.

## Suggested Next

Proceed to Step 6 validation and close-review preparation for the f128 shard
redistribution.

## Watchouts

- Keep the redistribution behavior-preserving.
- Preserve fail-closed and deferred-helper boundaries.
- Do not weaken tests, rewrite expectations, or add named-case f128 shortcuts.
- Do not redesign `instruction.hpp` solely to split this shard.
- Current live f128 support is prepared-fact driven and intentionally
  fail-closed: missing prepared carriers/helpers, incomplete 16-byte size or
  alignment, non-full-width carrier kinds for helper ABI moves, unsupported
  helper families, missing clobber/resource/live-preservation policies, missing
  selected-call ownership, unsupported scalar bridge widths, and unprintable
  memory/register facts all become diagnostics or target-unsupported output.
  Do not convert these checks into fallback scalar F64 behavior.
- Retired `f128.md` described legacy address/temp/constant/source hooks that are
  not currently implemented as live C++ selected nodes. Those old hook names,
  x16/x17 scratch/address choreography, q0 stack temp reservation, constant-half
  store helpers, and scalar-accumulator compatibility shims are non-live legacy
  or deferred implementation content unless future prepared facts and selected
  nodes model them explicitly.
- `src/backend/mir/aarch64/codegen/README.md` did not index `f128.md` as an
  active shard; it only preserves the historical `f128` module-list entry, so no
  README edit was needed for this packet.
- Transport, runtime-helper boundary instruction construction, and prepared
  runtime-helper boundary record construction are now owned by `f128.cpp`.
- `memory.cpp` should continue to own `lower_f128_transport_instruction`; it is
  the neutral memory router that prepares structured memory operands and calls
  f128-owned construction helpers.
- A narrow f128 transport lowering move was rejected for this packet because it
  would drag broad memory helper seams into `f128.cpp`.
- `make_f128_register_operand` no longer has a public `f128.hpp` declaration;
  keep it local to f128 construction unless a real external caller appears.
- Do not add broad neutral bank/class mapping helpers to `f128.cpp`; keep f128
  scalar register conversion explicit and fail-closed for unsupported banks.
- `f128.hpp` currently redeclares an existing surface from `instruction.hpp`;
  duplicate default arguments remain intentionally avoided there. Remove or
  narrow corresponding declarations in `instruction.hpp` only if needed for
  clean ownership and include flow.
- `f128_vector_register_name` is now f128-owned and declared in `f128.hpp`
  because the existing vector intrinsic printers still need the same q/vector
  spelling while remaining in generic printer dispatch.
- `clang-format` is not installed in this environment; the touched C++ files
  were left in the existing local style after manual review.

## Proof

Ran `git diff --check && test ! -e src/backend/mir/aarch64/codegen/f128.md && rg -n 'f128\.md' src/backend/mir/aarch64/codegen README.md docs ideas/open plan.md todo.md || true`.
The command completed successfully. `test_after.log` records that `f128.md` is
deleted and that remaining textual references are lifecycle/source-plan
references plus this Step 5 retirement note, not an active codegen shard index.
