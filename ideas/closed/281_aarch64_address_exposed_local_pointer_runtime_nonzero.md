# AArch64 Address-Exposed Local Pointer Runtime Nonzero

Status: Closed
Created: 2026-05-18
Discovered From: ideas/open/278_aarch64_backend_local_operand_materialization_runtime_nonzero.md
Closed: 2026-05-18

## Intent

Repair the next AArch64 backend runtime failure family exposed after local
integer operand materialization for `00003.c` was fixed: address-exposed local
and pointer semantics in `tests/c/external/c-testsuite/src/00004.c` and
`00005.c`.

## Why This Exists

Idea 278 repaired scalar local operand materialization for ordinary integer
arithmetic. The Step 3 broad AArch64 backend scan then advanced through
`00001.c`, `00002.c`, and `00003.c`, but `00004.c` and `00005.c` fail as
`[RUNTIME_NONZERO] exit=1`.

Those cases are later than the immediate local store/load materialization owner
and should not be folded back into idea 278. They need a focused owner-layer
trace for address-taken locals, pointer reads/writes, and the AArch64 memory
operand facts used by that path.

## In Scope

- Trace the AArch64 backend route for `00004.c` and `00005.c`.
- Identify whether the failure is in address-exposed local storage, pointer
  value materialization, load/store address formation, or prepared memory
  operand consumption.
- Repair the semantic lowering rule for the supported address-exposed local or
  pointer form proven by the trace.
- Preserve the idea 278 scalar local operand materialization behavior for
  `00003.c`.
- Prove the repair with focused backend coverage and the relevant AArch64
  backend c-testsuite cases.

## Out of Scope

- Reworking loop or branch control lowering for `00006.c`.
- Changing c-testsuite `.expected` sidecars, allowlists, unsupported
  classifications, or CTest expectations.
- Adding filename-specific handling for `00004.c` or `00005.c`.
- Routing proof through LLVM IR fallback while claiming AArch64 backend
  progress.
- Broad ABI, runtime-runner, or register-allocation rewrites unless tracing
  proves they are the direct owner of this address-exposed local family.

## Completion Criteria

- `00004.c` and `00005.c` advance through the AArch64 backend runtime route
  without expectation weakening.
- The generated AArch64 assembly for the repaired cases uses coherent
  address-exposed local or pointer storage semantics, not an unrelated
  incoming ABI register or filename-shaped shortcut.
- The idea 278 proof cases `00001.c`, `00002.c`, and `00003.c` remain green.
- Focused backend proof covers the repaired memory/pointer operand rule outside
  the exact c-testsuite filenames where practical.
- Any remaining later failure family is split into its own source idea instead
  of expanding this one.

## Closure Notes

Closed after Step 4 residual-scope review. The address-exposed local pointer
storage and memory-load return handoff scope is complete: `00004.c` passes the
AArch64 backend runtime route, the focused AArch64 backend memory/operand/
return proof is green, and `00001.c`, `00002.c`, and `00003.c` remain green.

The remaining `00005.c` failure was split into
`ideas/open/282_aarch64_loop_branch_control_runtime_hang.md` because its
pointer slot stores are coherent after this repair and the remaining evidence
is conditional branch/control lowering for the first `if`, not
address-exposed pointer storage. `00006.c` also remains owned by idea 282.

Close-time regression guard passed with matching-scope logs:
`test_before.log` and `test_after.log` both cover the six focused AArch64
backend tests plus `c_testsuite_aarch64_backend_src_(00001|00002|00003|00004)_c`,
with 4 passed, 0 failed, and no new failures in the parsed final CTest summary.

## Reviewer Reject Signals

Reject the route or slice if it:

- matches `00004.c`, `00005.c`, a c-testsuite path, or an exact emitted
  instruction sequence instead of repairing address-exposed local or pointer
  semantics;
- changes `.expected` files, allowlists, unsupported classifications, CTest
  contracts, runner files, or route diagnostics to claim progress;
- routes proof through LLVM IR fallback while claiming AArch64 backend progress;
- regresses the idea 278 scalar local operand materialization proof for
  `00003.c`;
- hides the pointer/address-exposed owner by making broad ABI,
  register-allocation, or runtime-runner rewrites without trace evidence;
- folds `00006.c` loop/branch control lowering into this idea instead of
  keeping it as a separate owner layer.
