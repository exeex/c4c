# Current Packet

Status: Active
Source Idea Path: ideas/open/334_object_route_scan_and_default_readiness.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Triage Remaining Object-Route Gaps

## Just Finished

- Completed Step 3 triage of remaining object-route scan candidates after the
  37/37 expanded baseline.
- Dry-ran representative backend and c-testsuite object candidates with
  temporary `/tmp` outputs only. No CLI, linker/toolchain, or runtime failure
  was the first blocker for the unsupported candidate families; failures were
  target object-emitter diagnostics before link.
- Target-emitter triage map:
  - RV64 local rewrite / parameter reassignment candidates
    (`param_slot.c`, `two_arg_*_rewrite.c`) fail at the RV64 prepared object
    predicate with `RISC-V backend object route unsupported prepared module
    shape`.
  - RV64 branch/control-flow candidates (`branch_if_eq.c`, `branch_if_ne.c`,
    `branch_if_lt.c`) fail at the same prepared object-shape gate.
  - RV64 pointer/global candidates (`local_pointer_deref.c`, `global_load.c`,
    `global_store.c`, c-testsuite `00004.c` through nearby pointer/local
    cases) fail at the same prepared object-shape gate.
  - RV64 has additional green object candidates (`local_array.c`, c-testsuite
    `00003.c`, `00011.c`, `00012.c`, and `00015.c`) that emit/link/run under
    qemu, but adding them now would create RV64-only scan growth while AArch64
    is blocked on the same c-testsuite frontier.
  - AArch64 local/frame-memory candidates (`param_slot.c`,
    `two_arg_*_rewrite.c`, c-testsuite `00003.c`, `00004.c`, `00011.c`,
    `00015.c`) fail in target object emission with selected fixed-frame /
    local-memory support missing. The generated asm uses stack allocation,
    `str`/`ldr`, and sometimes saved temporaries such as `x20`/`x21`.
  - AArch64 branch/control-flow and global candidates fail at selected-machine
    object emission with unsupported machine instructions.
  - AArch64 c-testsuite `00012.c` reaches scalar arithmetic but fails because
    the object emitter supports selected add/sub only; the generated asm uses
    `mul`.
- Owner classification:
  - Target encoder / object emitter: all current unsupported cases above.
  - Object writer / relocation: no first-failure evidence from this triage.
  - CLI: no first-failure evidence; diagnostics are stable `--codegen obj`
    target-backend failures.
  - Linker/toolchain/runtime: no first-failure evidence for unsupported cases;
    RV64 green candidates linked and ran when probed.
  - Policy/default readiness: still blocked until target-emitter frontiers are
    repaired or explicitly documented as out of default scope.

## Suggested Next

- Request plan-owner review/split before Step 4 default-readiness. More
  parent-level scan expansion can continue only as narrow RV64-only additions,
  but the balanced object-route/default-readiness path is blocked by target
  object-emitter capability gaps.
- Proposed focused child intent: expand target-owned object emission for the
  next c-testsuite/backend object frontier without changing CLI/default policy.
  First source files/cases should be:
  - AArch64 local/frame-memory object support for `tests/c/external/c-testsuite/src/00003.c`,
    `tests/c/external/c-testsuite/src/00011.c`, and backend `param_slot.c`
    or one `two_arg_*_rewrite.c` case.
  - AArch64 selected scalar multiply support for c-testsuite `00012.c`, if the
    local/frame-memory child does not naturally include it.
  - RV64 prepared object-shape expansion for `param_slot.c` /
    `two_arg_*_rewrite.c` and basic branch/control-flow only as separate
    substeps if the plan owner wants a multi-target child.
- Step 4 should not make a default-readiness recommendation until the plan owner
  either activates that focused child or explicitly narrows the default-readiness
  decision to the current 37-test smoke baseline.

## Watchouts

- Do not add expected-failure scan labels or weaken object/asm expectations.
- The c-testsuite object smokes added here are explicit backend tests under
  `tests/backend/CMakeLists.txt`; c-testsuite's own default/frontend and
  backend-asm helpers were not changed.
- Keep branch/control flow, local pointers, globals/data, aggregates, broad
  AArch64 runtime, x86 object output, object stdout, broader c-testsuite object
  defaults, and semantic-BIR object mode out of parent scan-registration
  packets unless a focused child repairs the target emitter first.
- Do not use RV64-only green candidates as default-readiness evidence for both
  targets. They are useful probes, but AArch64 blocks the same c-testsuite
  frontier at local/frame-memory and selected scalar instruction support.
- Label caveat remains: avoid bare `ctest -L object` for proof selection
  because it also matches labels containing `object`, including
  `aggregate_subobject`.

## Proof

- No build required for this triage-only packet.
- Ran targeted `./build/c4cll --codegen obj ...` dry-runs for representative
  backend and c-testsuite candidates, writing outputs only under `/tmp`.
- Ran `git diff --check`; result: passed.
