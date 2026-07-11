# LIR To BIR Adapter Boundary Umbrella

Status: Open
Type: Umbrella triage and follow-up idea generator
Parent: `none`
Handoff Directory: `docs/lir_bir_adapter_boundary/`
Related:
- `src/backend/bir/lir_to_bir.hpp`
- `src/backend/bir/lir_to_bir/lowering.hpp`
- `src/backend/bir/lir_to_bir/`
- `src/backend/bir/bir.hpp`
- `src/backend/prealloc/prealloc.cpp`
- `src/backend/prealloc/module.hpp`
- `docs/bir_core_cleanup/`
- `docs/bir_prealloc_fusion/`
- `docs/rv64_gcc_torture_post_contract/`
- `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv`
- `build/agent_state/rv64_gcc_c_torture_backend_failed.txt`

## Goal

Use the current RV64 GCC torture backend scan and existing BIR/prealloc design
notes to classify the `LIR -> BIR` adapter boundary, then generate ordered
follow-up ideas for interface cleanup before more backend case repair.

## Why This Exists

The latest RV64 GCC torture backend progress run reached `526 / 1467` passing
cases, with `941` failures still present. That count shows backend execution is
making progress, but the remaining failures are broad enough that direct
testcase repair risks adding more scattered adapter facts instead of clarifying
the interface.

The first cleanup priority is the `LIR -> BIR` boundary. The current
`src/backend/bir/lir_to_bir/lowering.hpp` surface exposes lowering context,
legacy type text parsing, structured layout fallback, global initializer
parsing, local/global address provenance, call ABI computation, and several
route-local compatibility maps in one detail header. That makes it hard to
tell whether a failure belongs to LIR import, canonical BIR semantics,
prepared/prealloc publication, or MIR consumption.

This umbrella exists to classify that boundary before implementation. It
should prevent route drift from "fix RV64 torture case N" into more
LIR-spelling, layout, initializer, or provenance shortcuts hidden behind BIR
names.

## Current Evidence

- Latest transient scan:
  `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv`, generated
  2026-07-10, reported `total=1467 passed=526 failed=941`.
- Latest transient failure list:
  `build/agent_state/rv64_gcc_c_torture_backend_failed.txt`.
- Public lowering entry:
  `src/backend/bir/lir_to_bir.hpp`.
- Adapter detail surface:
  `src/backend/bir/lir_to_bir/lowering.hpp`.
- Lowering implementation directory:
  `src/backend/bir/lir_to_bir/`.
- Existing BIR cleanup docs:
  `docs/bir_core_cleanup/`.
- Existing BIR/prealloc fusion docs:
  `docs/bir_prealloc_fusion/`.
- Existing RV64 post-contract BIR evidence:
  `docs/rv64_gcc_torture_post_contract/`.

The build paths above are evidence pointers only. The umbrella must write
durable summaries under `docs/lir_bir_adapter_boundary/` rather than treating
ignored `build/` artifacts as canonical lifecycle state.

## In Scope

- Create `docs/lir_bir_adapter_boundary/` as the durable handoff directory.
- Inventory the current `LIR -> BIR` public and detail interfaces.
- Classify exposed adapter responsibilities by first owning layer:
  LIR import, structured type/layout bridge, initializer bridge, memory and
  address provenance import, call ABI import, canonical BIR semantic model, and
  prepared/prealloc handoff.
- Identify which route-local compatibility maps are still required at import
  time and which should be hidden behind narrower adapter contracts.
- Compare the classification against existing `docs/bir_core_cleanup/`,
  `docs/bir_prealloc_fusion/`, and RV64 torture evidence.
- Generate ordered follow-up ideas under `ideas/open/` for behavior-preserving
  interface cleanup packets.
- Record dependency rules for which adapter contracts must be clarified before
  BIR core or MIR consumer changes.

## Out Of Scope

- Implementing cleanup directly inside this umbrella idea.
- Changing BIR lowering semantics, MIR output, runtime behavior, test
  expectations, unsupported markers, allowlists, timeout behavior, or default
  harness contracts.
- Moving target-specific RV64, AArch64, or x86 facts into the canonical BIR
  model.
- Treating transient files under `build/` as durable lifecycle notes.
- Rewriting `PreparedBirModule` or MIR consumers as part of this first
  boundary inventory.

## Priority Model

Order follow-up ideas by boundary clarity and blast-radius reduction:

1. Public/detail header separation for `LIR -> BIR` import contracts.
2. Structured type and legacy layout bridge isolation.
3. Global and aggregate initializer bridge isolation.
4. Local/global address provenance import boundary.
5. Call/return ABI import boundary.
6. Prepared/prealloc handoff cleanup only after the LIR import boundary has
   named stable input/output contracts.

Prefer behavior-preserving extraction and compile-proof packets before any
semantic repair. Prefer contracts that remove route-local raw spelling from
later layers over contracts that merely rename helpers.

## Required Follow-Up Ideas

Generate at least these follow-up families unless the inventory proves a
better split:

- `LIR import context extraction`: owning layer `LIR -> BIR adapter`.
- `Structured layout bridge isolation`: owning layer `LIR -> BIR adapter`.
- `Initializer lowering bridge isolation`: owning layer `LIR -> BIR adapter`.
- `Memory/address provenance import cleanup`: owning layer `LIR -> BIR adapter`.
- `Call ABI import boundary cleanup`: owning layer `LIR -> BIR adapter`.

Each generated follow-up must name the files it owns, the behavior-preserving
proof surface, and the downstream layers it must not edit.

## Acceptance Criteria

- `docs/lir_bir_adapter_boundary/` contains an interface inventory,
  responsibility classification, and ordered follow-up plan.
- The documents agree on the same current evidence source and clearly separate
  durable summaries from transient `build/` scan artifacts.
- Follow-up ideas are generated under `ideas/open/` and ordered by dependency.
- Each follow-up idea names its first owning layer and avoids mixing LIR import,
  canonical BIR semantics, prepared/prealloc publication, and MIR consumer
  changes.
- The umbrella does not change implementation, tests, expectations,
  unsupported markers, allowlists, runtime behavior, default harness
  contracts, or tracked build artifacts.

## Closure Note Requirements

The closure note must state which RV64 scan and existing docs were used, which
handoff documents were written, which follow-up ideas were generated, how they
were ordered, and which adapter responsibilities remain intentionally deferred.

## Reviewer Reject Signals

- Reject direct implementation inside the umbrella idea.
- Reject output that only lists files or counts without first-owner
  classification and ordered follow-up ideas.
- Reject treating ignored `build/` artifacts as canonical lifecycle state.
- Reject follow-up ideas that mix LIR import cleanup with BIR semantic changes,
  prepared/prealloc publication changes, or MIR consumer rewrites.
- Reject moving RV64/AArch64/x86 target-specific facts into canonical BIR as
  part of adapter cleanup.
- Reject testcase-shaped shortcuts, expectation rewrites, unsupported
  downgrades, allowlist filtering, weaker runtime checks, helper renames, or
  classification-only edits claimed as implementation progress.
- Reject retaining the current `lowering.hpp` responsibility pile behind a new
  abstraction name without reducing exposed adapter responsibilities.
