Status: Active
Source Idea Path: ideas/open/261_aarch64_f128_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Create The F128 Compiled Owner Boundary

# Current Packet

## Just Finished

Step 2: Create The F128 Compiled Owner Boundary completed as a
behavior-neutral build-boundary packet. Added `f128.hpp` with forwarding
declarations for the existing f128 transport/helper construction and naming
surface, added an empty compiled owner translation unit `f128.cpp`, and
registered `f128.cpp` in `src/backend/CMakeLists.txt`. No f128 bodies,
semantic checks, lowering routes, printer logic, tests, `plan.md`, source
idea, or markdown shard content moved or changed.

## Suggested Next

Step 3 should move the first narrow f128 implementation body cluster into
`f128.cpp`. The lowest-risk target remains transport construction:
`make_prepared_f128_carrier_transport_record`, its directly required f128-only
private helpers, and the transport kind/error naming helpers if dependency
shape stays local. Keep helper-boundary construction, lowering, and printer
spelling out of the first extraction unless required by compile ownership.

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
- `f128.hpp` currently redeclares an existing surface from `instruction.hpp`;
  duplicate default arguments are intentionally avoided there. When bodies move,
  remove or narrow the corresponding declarations in `instruction.hpp` only if
  needed for clean ownership and include flow.

## Proof

Exact delegated proof passed:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`.
`test_after.log` is preserved. The run built the new compiled owner and passed
139/139 backend tests.
