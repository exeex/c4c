Status: Active
Source Idea Path: ideas/open/250_i128_deferred_helper_family_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Deferred Helper Family Gaps

# Current Packet

## Just Finished

Executed Step 1 inspection for deferred i128 helper family authority.

Current surfaces:
- BIR/source opcode shapes are present for the deferred family: `bir::CastInst`
  carries `FPToSI`, `FPToUI`, `SIToFP`, `UIToFP`, `FPExt`, and `FPTrunc`; the
  LIR-to-BIR scalar lowerer preserves those cast opcodes structurally.
- Prepared i128 runtime helper mapping currently produces structural helper
  records only for i128 `SDiv`, `UDiv`, `SRem`, and `URem`. It records source
  block/instruction identity, source opcode, result/lhs/rhs value identities,
  helper family/kind, callee, direct low/high result ownership, carrier lanes,
  ABI bindings, marshaling/unmarshaling moves, clobber/resource policy,
  live-preservation policy, and selected-call ownership.
- The i128 float/integer conversion surface is explicitly detected but still
  function-level deferred through
  `i128_float_integer_conversion_helper_mapping_deferred`; no per-cast
  `PreparedI128RuntimeHelper` record exists yet for conversion source/result
  identity, signedness, widths, helper kind, or callee policy.
- Prepared helper data already has `FloatIntegerConversion` as a family value
  and `MemoryReturn` as both result-ownership and ABI-transition vocabulary,
  plus a `MemoryReturnOwnership` payload shape. The producer does not populate
  memory-return ownership for i128 helpers today; memory-return result policy
  still records `memory_return_ownership_requires_explicit_destination`.
- Generic prepared/selected call records already carry ordinary call
  memory-return storage, but that authority is not connected to i128 runtime
  helper records.
- AArch64 selected helper-boundary records fail closed outside complete
  direct-result div/rem: unsupported helper family, unsupported source
  operation, unsupported result ownership, incomplete prepared helper facts,
  missing ABI/resource/clobber policy, or incomplete selected-call ownership
  all reject the record. Terminal direct-result div/rem helper-call printing
  consumes the selected record plus the source prepared helper marshaling facts.

## Suggested Next

Execute Step 2 as a prepared/shared authority packet: add structural
`PreparedI128RuntimeHelper` mapping records for i128 float/integer conversion
casts that require runtime helpers, carrying function/block/instruction source
identity, cast opcode, source/result value identities, source/result type
facts, signedness/width facts, helper family/kind, and explicit callee/policy
identity where the source policy exists. Keep F128/binary128, float-width-only,
and memory-return-required shapes explicitly deferred unless their producer
policy is added in the same owned packet. Do not add AArch64 selected nodes,
terminal printer output, fixed-register marshaling, or direct-result div/rem
printer changes.

Suggested focused proof for Step 2:

```bash
(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_prepared_printer)$') > test_after.log 2>&1
```

## Watchouts

- Do not reopen direct-result div/rem helper-call printing unless inspection
  finds a concrete regression or missing shared fact.
- Do not treat the existing `FloatIntegerConversion` enum value as complete
  authority; helper kind/callee, source/result type semantics, ABI transition,
  marshaling, clobber/resource, live-preservation, and selected-call ownership
  still need producer-owned facts before AArch64 can consume conversion helpers.
- Do not treat the existing memory-return vocabulary as ownership. I128 helper
  memory-return support still lacks destination/slot/offset producer authority
  and helper ABI policy tying an sret/memory destination to the helper record.
- Do not hard-code helper ABI registers, fixed low/high lane pairs, register
  adjacency, rendered names, or helper callee strings in AArch64 target
  lowering.
- Do not lower helper-required operations as scalar i64 shortcuts.
- Preserve existing supported div/rem helper-call behavior while adding
  deferred helper authority.

## Proof

Inspection-only packet. No build or tests were run, and no proof log was
created or modified.

Read-only commands used:
- `git status --short`
- `sed -n '1,220p' todo.md`
- `c4c-clang-tool-ccdb list-symbols /workspaces/c4c/src/backend/prealloc/regalloc.cpp build/compile_commands.json`
- `c4c-clang-tool-ccdb list-symbols /workspaces/c4c/src/backend/prealloc/prealloc.cpp build/compile_commands.json`
- focused `rg`/`sed` reads under `src/backend/bir`, `src/backend/prealloc`,
  `src/backend/mir/aarch64/codegen`, `tests/backend/bir`, and
  `tests/backend/mir`
- `git diff --check`
