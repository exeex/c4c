# Current Packet

Status: Active
Source Idea Path: ideas/open/753_lir_memory_va_pointer_authority_convergence.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish and verify native pointer/object/lifetime authority

## Just Finished

-  753 Step 2 admitted AMD64 SysV scalar/pointer `va_arg` to native memory/VA
  authority only for a direct current-function `va_list` local carrying the
  existing complete pointer/object/owner/liveness tuple, with result value and
  type authority tied to `LirVaArgOp`. Indirect, incomplete, aggregate, and
  legacy routes remain compatibility-only. Added direct lowering coverage and
  incomplete, foreign, dead, result-mismatched, and type-mismatched verifier
  coverage.

## Suggested Next

- Supervisor: review and commit the completed 753 Step 2 scalar/pointer
  `va_arg` authority slice, then select the next bounded Step 2 producer packet.

## Watchouts

- Native aggregate-zero local `memset` authority requires a positive known
  size. memcpy remains historical selected-only; builtin/indirect VA and
  `va_arg` aggregate/memcpy-like, RV64, AArch64, legacy, and non-direct routes
  remain compatibility-only. `va_copy` requires both direct-local operands; a
  one-sided or non-direct form must stay compatibility-only. Do not derive
  facts from display text or widen the historical memcpy descriptor.

## Proof

- Scalar/pointer `va_arg` authority proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
  passed 5/5; `test_after.log` is the proof log. `git diff --check` remains
  required before commit.
