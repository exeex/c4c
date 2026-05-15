# AArch64 Intrinsic Machine Nodes

Status: Closed
Created: 2026-05-14
Closed: 2026-05-15

Parent Context: ideas/open/229_aarch64_codegen_markdown_shards_to_cpp.md

## Problem

The archived `intrinsics.md` describes multiple target-specific intrinsic
families: barriers, cache and pause hints, non-temporal memory placeholders,
128-bit vector operations, CRC instructions, frame/return/thread address
builtins, and scalar floating-point unary operations. Current shared lowering
has limited runtime and intrinsic placeholder support, including inline asm and
some `fabs` lowering into BIR calls, but compiled AArch64 codegen does not
select or print target-specific intrinsic machine nodes.

The valid intrinsic families need structured ownership. They should not be
recreated as accumulator-based local snippets or silent x86-only zero-fill
fallbacks.

## Scope

- Define AArch64 intrinsic-family carriers for accepted barrier/cache/hint,
  scalar FP unary, CRC, builtin address, vector memory, and vector operation
  cases.
- Lower selected machine nodes from prepared/shared facts where the semantic
  carrier already exists; create separate dependency ideas where it does not.
- Add printer support only for structured AArch64 intrinsic nodes with explicit
  feature and operand contracts.
- Keep binary128 helper calls delegated to the binary128 soft-float route.

## Non-Goals

- Do not treat all x86 SSE/AES/CLMUL intrinsics as silently supported on
  AArch64.
- Do not rebuild the archived `x0`/`w9`/`q0`/`q1` scratch-register convention.
- Do not hide missing vector, CRC, or feature-gating facts behind text
  emission.
- Do not merge this route with ordinary call/frame machine-node work.

## Proof Direction

- A scalar FP unary intrinsic lowers through typed FP/SIMD operands rather than
  raw integer accumulator moves.
- Barrier, pause, cache-maintenance, and CRC intrinsics emit only when their
  structured feature and operand facts are present.
- Vector intrinsic cases preserve the intended 128-bit operand and destination
  contract without named-case string matching.
- Unsupported x86-only intrinsics are rejected, trapped, or explicitly
  diagnosed by policy rather than silently zero-filled.

## Lifecycle Checkpoint - 2026-05-15

The repaired scalar runbook for this idea is complete and retired, but this
source idea remains open.

Completed scalar route:

- Structured prepared carriers now cover scalar FP unary F32/F64 `fabs`.
- AArch64 selection consumes those complete carriers into selected scalar
  intrinsic machine records.
- The machine printer emits scalar `fabs` assembly only from selected records
  with explicit operand/result register authority.
- Unsupported x86-only, F128, missing-feature, missing-operand, incomplete
  carrier, and non-selected intrinsic paths fail closed with diagnostics rather
  than fabricated registers, zero-fill output, or intrinsic-name matching.

Dependency split:

- CRC, vector memory, and vector operation carrier authority is tracked by
  `ideas/closed/241_aarch64_crc_vector_intrinsic_carriers.md`.
- Those families must not be selected or printed by this idea until complete
  semantic and prepared carrier facts exist.

Remaining source scope:

- Barrier, cache, pause/hint, builtin-address, CRC, vector memory, and vector
  operation machine-node selection/printing remain open unless a future
  runbook proves complete structured carrier authority for each family.
- Binary128 helpers remain delegated outside this idea.

Latest broad backend proof for the retired scalar runbook:

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
- Result: 139/139 backend tests passed in `test_before.log`.

## Closure Note - 2026-05-15

Closed after the second active runbook completed the machine-node route for
the intrinsic families with complete carrier authority.

Completed machine-node support:

- Scalar F32/F64 `fabs` selection and printing from selected records remained
  intact.
- CRC32W, vector load, and vector add now select from complete prepared
  intrinsic carriers into AArch64 machine records.
- The machine printer emits `crc32w`, `ld1 {vN.16b}, [xN]`, and `add
  vD.16b, vL.16b, vR.16b` only from selected structured records with explicit
  register and shape authority.
- Unsupported x86-only, incomplete-carrier, malformed-vector-shape,
  non-selected, and fabricated intrinsic-shaped paths remain fail-closed.

Dependency split:

- Barrier, cache-maintenance, pause/hint, and builtin-address intrinsic
  families still lack complete BIR semantic intrinsic facts plus prepared
  intrinsic carrier authority.
- That upstream work is split to
  `ideas/open/242_aarch64_barrier_cache_hint_builtin_intrinsic_carriers.md`.
- Machine-node selection or printer support for those families must wait for
  that dependency route or a later explicit follow-on machine-node idea.

Close proof:

- Supervisor acceptance after commit `e6d4a1216` passed
  `cmake --build --preset default`, the narrow intrinsic backend subset, and
  `ctest --test-dir build -j --output-on-failure -R '^backend_'`.
- Plan-owner close gate passed with matching backend before/after logs:
  139/139 backend tests passed.
