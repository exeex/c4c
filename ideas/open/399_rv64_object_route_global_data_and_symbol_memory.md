# RV64 Object Route Global Data And Symbol Memory

Status: Open
Type: Follow-up repair idea
Parent: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`

## Goal

Repair current RV64 object-route failures for prepared global data, global
memory facts, and direct global-symbol base-plus-offset addressing.

## Why This Exists

The 2026-06-26 reopened 354 classification found 30 global-data failures:

- 15 `RV64 object route supports only immediate scalar and immediate linear global storage`
- 5 `RV64 object route supports only 1-, 2-, 4-, and 8-byte prepared global memory accesses`
- 5 `RV64 object route requires supported prepared global memory facts`
- 5 `RV64 object route requires prepared direct global-symbol base-plus-offset memory addressing`

Representatives:

- `tests/c/external/gcc_torture/src/20010924-1.c`
- `tests/c/external/gcc_torture/src/20001121-1.c`
- `tests/c/external/gcc_torture/src/20031211-1.c`
- `tests/c/external/gcc_torture/src/pr57568.c`

## In Scope

- Classify the remaining prepared global storage and global-memory facts.
- Add RV64 ELF/data/relocation lowering for supported prepared facts.
- Split missing producer-side global initializer or memory facts into a
  separate upstream idea when needed.
- If global initializer, symbol-memory, or base-plus-offset facts are missing
  from BIR/prepared output, route that to a separate BIR/prepared idea instead
  of inferring source/global semantics in MIR/RV64 object emission.

## Out Of Scope

- Rewriting frontend constant handling as part of RV64 object emission.
- Reclassifying unsupported globals as unsupported tests.
- Broad data-section rewrites unrelated to the current prepared global facts.

## Acceptance

- Representative global-data cases no longer fail with the same global-data
  diagnostics.
- Newly emitted global data links and passes qemu comparison where the case is
  runnable.
- Remaining global failures are either reduced in count or split by concrete
  producer-side missing facts.

## Reviewer Reject Signals

- Reject source-name matching or special handling for the representative cases.
- Reject object emission that creates malformed ELF data or relocations while
  merely clearing the compile diagnostic.
- Reject moving frontend/global-initializer semantics into RV64 target code.
- Reject MIR/RV64 changes that reconstruct global initializer bytes, symbol
  identity, or memory-access facts that should be owned by BIR/prepared
  publication.
- Reject expectation rewrites or allowlist filtering.
