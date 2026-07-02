Status: Active
Source Idea Path: ideas/open/558_bir_call_metadata_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Direct-Call Producer Boundary

# Current Packet

## Just Finished

Step 1 - Inspect Direct-Call Producer Boundary completed for
`src/20000412-2.c`.

Evidence used:
`build/agent_state/558_step1_20000412.log` and
`build/rv64_gcc_c_torture_backend/src_20000412-2.c/case.log` both report
semantic `lir_to_bir` failure in `main` under `direct-call semantic family`.
`build/c4cll --target riscv64-linux-gnu --codegen llvm
tests/c/external/gcc_torture/src/20000412-2.c -o
build/agent_state/558_step1_20000412.ll` shows the failing `main` call shape:
`%t0 = call i32 (i32, ptr) @f(i32 100, ptr null)`. `--dump-bir` fails at the
same semantic call-family gate before producing BIR.

Boundary: direct-call lowering has enough same-module callee metadata to treat
`@f` as a metadata-rich direct call with structured signature, but its pointer
argument path rejects typed null pointer operands. In
`src/backend/bir/lir_to_bir/calling.cpp`, pointer parameters call
`lower_public_pointer_call_arg_value`; that falls through to
`lower_call_pointer_arg_value` in
`src/backend/bir/lir_to_bir/memory/provenance.cpp`. That helper admits SSA
local/aggregate pointers and global/function symbols, but returns `nullopt`
for non-global, non-SSA operands, so `ptr null` never reaches the existing
`lower_value(..., TypeKind::Ptr, ...)` null-pointer support. The missing BIR
metadata is therefore the direct-call argument value/source publication for a
structured pointer parameter whose source operand is `null`: expected BIR
should publish the `CallInst` with callee LinkNameId/signature identity,
`arg_types[1] == Ptr`, pointer ABI metadata, an immediate null `Value`, and an
`Immediate` call-argument source relationship.

## Suggested Next

Add focused BIR coverage in `tests/backend/bir/backend_lir_to_bir_notes_test.cpp`
for a metadata-rich same-module/direct call with a structured `(i32, ptr)`
callee signature and `ptr null` argument. Suggested test name:
`expect_metadata_rich_direct_call_null_pointer_argument_publishes_immediate_source`.
Then repair the producer path so fixed direct-call pointer arguments can use
existing pointer null lowering and publish the immediate source relationship.

## Watchouts

Reject downstream RV64/MIR call inference, generic local-memory routing,
runtime/intrinsic repairs, expectation rewrites, unsupported-marker changes,
allowlist edits, runtime-comparison changes, and named-case shortcuts. The
runbook must cover call-return metadata before claiming the source idea is
complete. After `ptr null` is admitted, the same representative may expose the
recursive `f(a-1, &x)` frame-slot address argument next; keep that as the next
producer boundary unless the direct null-pointer repair also covers it through
general pointer argument publication.

## Proof

Inspection-only packet. Commands/evidence:

- `build/c4cll --target riscv64-linux-gnu --codegen llvm
  tests/c/external/gcc_torture/src/20000412-2.c -o
  build/agent_state/558_step1_20000412.ll`
- `build/c4cll --target riscv64-linux-gnu --dump-bir
  tests/c/external/gcc_torture/src/20000412-2.c`, which failed at the expected
  direct-call semantic admission gate.
- Source inspection of `src/backend/bir/lir_to_bir/calling.cpp`,
  `src/backend/bir/lir_to_bir/memory/provenance.cpp`, and existing focused
  BIR call tests.

Post-repair representative proof command:

`ALLOWLIST=build/agent_state/558_step1_20000412.allowlist VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh`

No code build required for this inspection packet. `git diff --check -- todo.md`
passed.
