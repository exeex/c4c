# RV64 Prepared Object Shape Diagnostics

Status: Open
Type: Repair idea
Parent: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`

## Goal

Replace the coarse RV64 object-route `unsupported prepared module shape`
diagnostic with structured rejection reasons that identify the failing prepared
module/function/instruction family.

## Why This Exists

The RV64 gcc torture backend scan found `1012` failures collapsed into one
message. Analysis shows at least nine buckets: module data, multi-block
control flow, call ABI, local memory width, variadic entry, declaration
control-flow entries, FPR ABI, and local memory addressing. Without structured
diagnostics, every later backend repair has to rediscover the hidden gate.

Relevant code:

- `src/backend/backend.cpp`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/prepared_module_emit.cpp`

## In Scope

- Introduce a structured RV64 prepared-object rejection result, or an
  equivalent diagnostic collector, instead of returning bare `std::nullopt`.
- Preserve the current user-facing failure mode while appending actionable
  details such as `module_data_string_constants`,
  `multi_block_control_flow`, `unsupported_binary_opcode`,
  `local_memory_width`, or `declaration_control_flow_entry`.
- Add narrow diagnostics tests that prove representative prepared-shape cases
  report stable buckets.
- Keep diagnostics semantic and stage-oriented; do not classify by torture
  testcase name.

## Out of Scope

- Implementing the backend capability repairs.
- Weakening the RV64 gcc torture runner.
- Marking prepared-shape cases unsupported.
- Rewriting BIR/MIR lowering semantics.

## Representative Cases

- `src/20000113-1.c`: multi-block control flow.
- `src/20000224-1.c`: global load/store and global address data.
- `src/20000112-1.c`: string constants.
- `src/20001203-1.c`: non-i32 local memory widths.
- `src/20030216-1.c`: declaration control-flow entry.

## Suspected Stage

RV64 prepared object admission and object emission diagnostics.

## Proof Command

```sh
CASE_TIMEOUT_SEC=20 MAX_CASES=0 scripts/check_progress_rv64_gcc_c_torture_backend.sh
```

For narrow iteration:

```sh
build/c4cll -I tests/c/external/gcc_torture --codegen obj \
  --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20000113-1.c \
  -o /tmp/rv64-prepared-shape.o
```

The narrow command should fail with a structured RV64 prepared-object reason,
not only the old generic message.

## Acceptance Criteria

- The representative cases each report a stable semantic rejection bucket.
- The full scan can be bucketed from logs without re-dumping prepared BIR.
- Existing passing RV64 object runtime cases still pass.
- The generic message may remain as a prefix, but it must no longer be the only
  actionable diagnostic.

## Reviewer Reject Signals

- Reject if the implementation maps testcase names directly to buckets.
- Reject if diagnostics are produced only by the gcc torture script rather than
  the backend/object route.
- Reject if existing passing object cases regress.
- Reject if the change hides the failure by downgrading tests, changing the
  runner contract, or removing `--codegen obj` coverage.
- Reject if the exact old failure mode is merely renamed without identifying
  the failing module/function/instruction shape.

