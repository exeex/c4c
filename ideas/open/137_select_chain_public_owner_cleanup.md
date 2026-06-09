# 137 Select-chain public owner cleanup

## Goal

Give select-chain dependency queries one public owner and one include boundary
without changing the prepared facts they compute.

## Why This Exists

The Step 3 classification in the BIR/prealloc prepared query surface audit
found that select-chain dependency facts are reusable shared semantics, but the
public API is split too narrowly:

- declarations for `PreparedSelectChainDependencyQuery` and
  `find_prepared_direct_global_select_chain_dependency` appear through
  `src/backend/prealloc/prepared_lookups.hpp`
- related declarations for
  `find_prepared_select_chain_source_producer`,
  `find_prepared_store_source_direct_global_select_chain_dependency`, and
  `find_prepared_scalar_select_chain_materialization` appear through
  `src/backend/prealloc/publication_plans.hpp`
- definitions already live in
  `src/backend/prealloc/select_chain_lookups.cpp`

Future x86/riscv code should discover select-chain dependency facts from a
single domain owner instead of depending on accidental facade headers.

## In Scope

- Select-chain public declarations and includes in:
  - `src/backend/prealloc/prepared_lookups.hpp`
  - `src/backend/prealloc/publication_plans.hpp`
  - `src/backend/prealloc/select_chain_lookups.cpp`
- Call sites that currently include the wrong public owner for:
  - `PreparedSelectChainDependencyQuery`
  - `find_prepared_direct_global_select_chain_dependency`
  - `find_prepared_select_chain_source_producer`
  - `find_prepared_store_source_direct_global_select_chain_dependency`
  - `find_prepared_scalar_select_chain_materialization`
- Known consumer groups to keep compiling:
  - `src/backend/prealloc/call_plans.cpp`
  - `src/backend/prealloc/publication_plans.cpp`
  - `src/backend/prealloc/prepared_printer/select_chains.cpp`
  - `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
  - `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
  - `src/backend/mir/aarch64/codegen/alu.cpp`
  - `src/backend/mir/aarch64/codegen/memory.cpp`
  - `src/backend/mir/aarch64/codegen/calls.cpp`

## Out Of Scope

- Changing select-chain dependency semantics.
- Rewriting AArch64 select materialization, store lowering, call lowering, or
  producer dispatch.
- Deleting shared select-chain facts and making consumers rescan BIR producers.
- Moving store-source publication planning or call-argument planning into the
  select-chain module.

## Acceptance Criteria

- Select-chain query declarations have one stable public owner, preferably a
  small select-chain lookup header paired with `select_chain_lookups.cpp`, or a
  clearly documented domain owner if the existing layout makes a new header too
  costly.
- `prepared_lookups.hpp` and `publication_plans.hpp` no longer both expose the
  same select-chain dependency surface.
- AArch64 consumers compile against the owner header and do not gain local
  source-producer scans.
- Public names remain semantic: direct-global dependency, source producer, and
  scalar materialization facts stay target-neutral.

## Proof Route

- `cmake --build --preset default`
- `ctest --test-dir build -R '^backend_' --output-on-failure`
- Broaden to full `ctest --test-dir build -j --output-on-failure` only if the
  implementation changes select-chain facts rather than include/API ownership.

## Reviewer Reject Signals

- Reject if the diff changes result fields such as
  `contains_direct_global_load`, `root_is_select`, or
  `root_instruction_index` without a semantic bug being proven.
- Reject if AArch64 code starts walking local BIR producers to replace
  `find_prepared_*select_chain*` queries.
- Reject if this becomes a broad publication-plan or call-plan rewrite instead
  of a public-owner cleanup.
- Reject if test expectations are weakened, backend tests are marked
  unsupported, or select-chain diagnostics are rewritten as proof of progress.
- Reject if the only result is helper renaming while the duplicate public
  ownership between `prepared_lookups.hpp` and `publication_plans.hpp` remains.
