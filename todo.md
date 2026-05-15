Status: Active
Source Idea Path: ideas/open/242_aarch64_barrier_cache_hint_builtin_intrinsic_carriers.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Current Carrier Gaps

# Current Packet

## Just Finished

Plan Step 1 - Inventory Current Carrier Gaps completed.

Current carrier surface:

- BIR intrinsic authority is limited to `ScalarFpUnary`, `Crc`,
  `VectorMemory`, and `VectorOperation` families, with operations `FAbs`,
  `Crc32W`, `VectorLoad`, and `VectorAdd`.
- Prepared intrinsic carriers copy BIR intrinsic facts into
  `PreparedIntrinsicCarrier`, but validation only accepts scalar fabs, CRC32W,
  vector load, and vector add. Other families cannot become complete carriers.
- The prepared printer can expose family, operation, feature, operand/result
  types, roles, vector shape, signedness, memory access, memory size/alignment,
  side effects, call-plan presence, source callee, operands, and homes for
  complete carriers only.
- AArch64 dispatch consumes only complete scalar-fp-unary, CRC32W,
  vector-load, and vector-add prepared intrinsic carriers. It has no selected
  barrier, cache-maintenance, pause/hint, or builtin-address records.

Per-family readiness:

- Barrier representative: first implementable as a semantic carrier for an
  AArch64 barrier intrinsic such as `llvm.aarch64.dmb`/`dsb`/`isb`.
  Missing fields are a barrier family, operation enum, barrier domain/kind
  immediate facts, side-effect/ordering facts, void-result handling, operand
  role for required immediates, target feature/policy, prepared validator, and
  printer proof. Atomic fence carriers already model generic ordering, but are
  not intrinsic-family authority and should not be reused as selected support.
- Cache-maintenance representative: first implementable after barrier or in a
  sibling packet as an AArch64 cache intrinsic such as `dc`/`ic` maintenance.
  Missing fields are a cache-maintenance family, operation enum, pointer
  operand role, address-space/memory operand facts, read/write/invalidate
  memory-access semantics, required immediate/cache-op selector facts, side
  effects, prepared pointer home validation, and printer proof.
- Pause/hint representative: implementable as a small no-result, side-effecting
  semantic carrier for an AArch64 hint/pause intrinsic once immediate
  operation facts exist. Missing fields are a hint family, operation enum,
  hint immediate/kind, side-effect facts, void-result handling, optional
  target feature/policy, prepared validation, and printer proof. It must not be
  inferred from rendered `hint`/`yield` text.
- Builtin-address representative: blocked from the same first packet because
  prepared address materialization and TLS/thread-pointer support are separate
  carriers, not intrinsic-family authority. Missing fields are a builtin-address
  family, operation enum for return/frame/thread pointer style cases, level or
  selector immediate facts, result pointer type, frame/return/TLS provenance,
  result home validation, and printer proof.

First implementable carrier packet: add BIR semantic facts for one no-result
AArch64 barrier representative, preferably a single barrier operation with an
immediate/domain fact, plus fail-closed malformed/target-mismatched cases. That
packet should not add selected-machine records.

## Suggested Next

Proceed to `plan.md` Step 2 for the barrier representative only: define the
structured BIR intrinsic family/operation/immediate/side-effect facts and the
nearby fail-closed tests, leaving prepared carriers and AArch64 dispatch
unchanged.

## Watchouts

- Do not select or print AArch64 machine records for these families in this
  carrier plan.
- Keep atomic fence carriers and prepared address materialization distinct from
  intrinsic-family carrier authority.
- Treat x86-only, target-mismatched, malformed, missing-home, and incomplete
  paths as fail-closed diagnostics.
- Preserve existing scalar, CRC, and vector intrinsic behavior while extending
  the carrier surfaces.
- The current BIR `IntrinsicOperation` shape has immediate booleans but no
  stored immediate value/domain, so Step 2 likely needs a real semantic field
  rather than name-only recognition.
- Void-result intrinsics need explicit prepared validation; current intrinsic
  validators all assume a result for the accepted families.

## Proof

Delegated proof for the inventory packet:

- `set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(lir_to_bir_notes|prepared_printer|aarch64_instruction_dispatch)'; } 2>&1 | tee test_after.log`
- Proof log: `test_after.log`
