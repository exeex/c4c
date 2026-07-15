# LIR PHI Incoming Producer Authority Repair

Status: Open
Type: bounded PHI producer-handoff baseline blocker
Blocked Parent: `ideas/open/754_lir_aggregate_vector_value_identity_convergence.md`

## Goal

Restore native current-function `LirValueId` authority for the failing PHI
incoming producer handoff exposed by the full baseline, without reopening the
accepted CFG/PHI schema or verifier contract.

## Why This Exists

At clean HEAD, the mandatory full baseline for 754 stopped at 1447/3037 on
`llvm_gcc_c_torture_src_vrp_2_c` with
`LirPhiIncoming.value: must identify a known current-function LirValueId`.
The clean build succeeded. `git blame` locates the relevant verifier
enforcement in `6ece9fe8f` (`Publish typed LIR PHI incoming authority`), but
this idea does not assume that commit is the root cause.

## In Scope

- Trace the single failing PHI incoming producer-to-consumer handoff and state
  the exact missing, stale, or foreign native authority fact.
- Make the smallest producer-side or immediate lowering-handoff repair that
  supplies a valid current-function `LirValueId` to the existing PHI contract.
- Add nearby same-family positive and malformed-authority coverage, then prove
  the bounded route before returning control to 754's full-baseline gate.

## Out Of Scope

- Reopening or weakening accepted CFG/PHI schema, edge, predecessor, or
  verifier authority.
- Aggregate/vector `LirExtractValueOp` work, Raw-BIR, pointer/object/memory,
  generic expression provenance, rendered-text recovery, or a broad residual
  instruction/terminator sweep.

## Acceptance Criteria

- The traced PHI incoming handoff supplies checked native current-function
  value authority to the existing contract.
- Missing, unknown, foreign, and stale authority remains rejected.
- Focused same-feature proof passes, and the supervisor can resume 754's
  required 100% full-baseline gate without changing its scope.

## Reviewer Reject Signals

- Reject any change that weakens `LirPhiIncoming.value` verification or permits
  raw/display text as identity merely to make `vrp_2.c` pass.
- Reject reopening accepted CFG/PHI predecessor or edge semantics, generic
  instruction/terminator conversions, or a catch-all provenance rewrite.
- Reject testcase-shaped special cases, expectation downgrades, or a green
  named test without nearby malformed-authority coverage.
- Reject claiming `6ece9fe8f` is causal without producer-handoff evidence.

## Resumption Record — switched to 805

- **Last accepted progress:** Steps 1 and 2 are complete. The traced seam was
  scalar integer unary minus in the ternary else incoming; commit `308fff39c`
  publishes its `sub` result through `fresh_value(ctx)` so the incoming retains
  a native current-function `LirValueId`.
- **Interrupted step:** Step 3 — *Prove the blocker and return it to 754*.
- **Outside-scope blocker:** the checkout has unresolved three-way integration
  conflicts in BIR container/global core files (`src/backend/bir/core/{builder.cpp,builder.hpp,ids.hpp,ir.hpp,view.hpp}` and
  `src/backend/bir/verify/{verifier.cpp,verifier.hpp}`). Resolving that shared
  BIR integration family is not PHI producer-to-incoming authority work and is
  owned by `ideas/open/805_bir_core_container_global_integration_conflict_repair.md`.
- **Exact return point:** once 805 restores a coherent checkout and its
  acceptance evidence is recorded, reactivate this source at **Step 3**;
  preserve Steps 1–2 and do not rerun their implementation route.
- **Remaining action:** obtain a fresh build and focused same-feature proof,
  then have the supervisor run and accept the required 100% full baseline
  before reactivating 754 at its unchanged Step 2.
- **Accepted evidence:** `308fff39c`; the recorded Step-2 proof is
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^llvm_gcc_c_torture_src_vrp_2_c$' > test_after.log`
  (passed), plus a passing
  `build/tests/backend/bir/backend_lir_to_bir_interface_test` exercising the
  positive and malformed incoming-authority cases. No new proof is valid while
  the integration conflicts remain.

## Resumption Update — residual PHI producer-family baseline blocker

- **Last accepted progress:** Steps 1 and 2 remain accepted. The only selected
  seam is scalar integer unary minus in the ternary else incoming, repaired in
  `308fff39c` by publishing the `sub` result through `fresh_value(ctx)`.
  The retained focused `vrp_2.c` proof remains passing, and 805 subsequently
  restored a buildable integration checkout with its recorded backend proof.
- **Interrupted step:** Step 3 — *Prove the blocker and return it to 754*.
- **Baseline result:** the fresh supervisor command `cmake --build --preset
  default && ctest --test-dir build -j --output-on-failure > test_after.log`
  built successfully but completed only 3033/3037. The four failures are
  `ieee/pr50310.c`, `20000715-1.c`, `20060910-1.c`, and `pr68376-2.c`; each
  reports `LirPhiIncoming.value: must identify a known current-function
  LirValueId`.
- **Classification:** `separate-blocker`. A common verifier diagnostic is not
  evidence that these cases traverse the exact scalar unary-minus handoff
  selected by this source, and the retained `vrp_2.c` proof already covers that
  handoff. Tracing another producer family would expand this source beyond its
  expressly bounded unary-minus route and its rejection of a broad residual
  instruction/terminator sweep.
- **Successor and exact return point:**
  `ideas/closed/806_lir_phi_residual_producer_family_authority_trace.md` owned
  only classification and, if evidenced, the smallest repair for the four
  observed residual cases. After it supplies an accepted same-feature repair
  and the supervisor accepts a 100% full baseline, reactivate 804 at unchanged
  Step 3. Do not repeat Steps 1–2, reopen CFG/PHI semantics, or turn 804 into
  a residual-family sweep.
- **Proof references:** `308fff39c`; retained focused `vrp_2.c` proof; 805's
  fresh build and `^backend_` 5/5 proof; the rejected 3033/3037 full baseline
  in root `test_after.log`. The original matching `test_before.log` is
  unavailable because the initial checkout was unmerged.

## Resumption Update — 806 chain accepted

- **Last accepted progress:** Steps 1--2 remain accepted. The selected scalar
  integer unary-minus producer handoff was repaired in `308fff39c`.
- **Interrupted step:** Step 3 — *Prove the blocker and return it to 754*.
- **Resolved successor chain:** 806 completed its residual producer-family
  route: postfix in `961ce9fda`, separately closed `fneg` and `xor` successors
  in `8f31e2535` and `b86df3b9d`, and scalar dereference-load authority in
  `4d29f7b3e`.
- **Return evidence:** the supervisor accepted a fresh full baseline at
  3037/3037 from `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure` (build had no work; total about 29.11s). That fulfills
  806's sole remaining Step 3 gate.
- **Exact return point:** resume this source at unchanged Step 3 only. Do not
  repeat Steps 1--2 or reopen CFG/PHI semantics or the closed residual routes.
