Status: Active
Source Idea Path: ideas/open/261_aarch64_f128_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit F128 Ownership And Build Surface

# Current Packet

## Just Finished

Step 1: Audit F128 Ownership And Build Surface completed as an audit-only
packet. Existing AArch64 f128 behavior is split across these live owners:

- Selected-node construction in `instruction.hpp` / `instruction.cpp`: f128
  owns record types and helpers for `F128TransportRecord`,
  `F128RuntimeHelperBoundaryRecord`, `make_f128_transport_instruction`,
  `make_f128_runtime_helper_boundary_instruction`,
  `make_prepared_f128_carrier_transport_record`, and
  `make_prepared_f128_runtime_helper_boundary_record`.
- Helper-call transport in `instruction.cpp`: runtime-helper construction
  converts prepared f128 carriers into q-register operands, validates
  arithmetic/comparison/cast helper families, records ABI transition policy,
  records resource/clobber/live-preservation policy, and materializes full
  width carrier moves plus scalar F32/F64 or I32 comparison result bridges.
- Source tracking in `instruction.hpp` / `instruction.cpp`: f128 transport and
  helper records retain `source_carrier` and `source_helper` provenance from
  prealloc prepared facts. This is the compiled successor to the markdown
  shard's full-source-tracking contract; relocation must preserve these
  provenance pointers and all missing/incomplete-prepared-fact diagnostics.
- Lowering in `dispatch.cpp` and `memory.cpp`: helper lowering is currently in
  `dispatch.cpp` via `lower_f128_runtime_helper_instruction`, while load/store
  transport lowering is already in the broad memory owner through
  `lower_f128_transport_instruction`. Dispatch routes f128 helper candidates
  before scalar float/cast fallback; memory routes F128 local loads/stores
  through structured f128 transport before ordinary memory lowering.
- Spelling in `machine_printer.cpp` plus comparison support in
  `comparison.cpp`: f128 q/vector/scalar register naming, helper marshal
  spelling, comparison-result validation, `ldr q` / `str q` transport, `mov
  vN.16b`, `fmov`, `bl`, `cmp`, and `cset` emission are still owned by the
  generic printer/comparison files.
- Markdown-only legacy content in `f128.md`: older soft-float hooks involving
  `x16`/`x17`, temp stack slots, constant halves, indirect address materializer
  shims, and accumulator cache hooks are not live codegen bodies in the current
  C++ surface. Treat them as constraints and deferred rebuild guidance, not
  behavior to invent during redistribution.
- Build integration: AArch64 codegen sources are explicitly listed in
  `src/backend/CMakeLists.txt`; Step 2 should add
  `"${CMAKE_CURRENT_SOURCE_DIR}/mir/aarch64/codegen/f128.cpp"` to
  `C4C_BACKEND_SOURCES`. No per-directory `codegen/CMakeLists.txt` exists.

## Suggested Next

Step 2 should create `src/backend/mir/aarch64/codegen/f128.hpp` and
`src/backend/mir/aarch64/codegen/f128.cpp`, register `f128.cpp` in
`src/backend/CMakeLists.txt`, and add only behavior-neutral forwarding
declarations. The lowest-risk first boundary is to declare f128-owned helpers
for the existing compiled surfaces without moving bodies yet:
`make_prepared_f128_carrier_transport_record`,
`make_prepared_f128_runtime_helper_boundary_record`, f128 transport/helper kind
names and error names if dependency shape allows, and printer-facing helper
declarations only if needed later. Keep Step 2 build-only/forwarding-only; defer
body movement to Step 3.

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
- `f128.md` describes legacy address/temp/constant/source hooks that are not
  currently implemented as live C++ selected nodes. Moving code must not
  synthesize those old hooks or claim semantic completion by recreating names.
- The first real body move should probably split transport construction from
  helper-boundary construction; moving both helper construction and printer
  spelling at once would cross more dependencies than necessary.
- `memory.cpp` already owns `lower_f128_transport_instruction`; Step 3 needs to
  decide whether that lowering is routed through `f128.cpp` or left as neutral
  memory routing that calls f128-owned construction helpers.

## Proof

Audit-only lifecycle packet; no build or test run required by the delegated
proof. Read-only inspection commands used: `git status --short`, `sed` on
`AGENTS.md`, `plan.md`, `todo.md`, `f128.md`, selected codegen/build files,
`rg` over AArch64 f128 symbols, `rg --files` under AArch64 codegen, and `find`
for CMake/build files. No `test_after.log` was created because the packet
explicitly exempted builds/tests and root logs.
