# LIR Cross-Function `LirValueId` Ownership Restoration

Status: Closed — capability complete
Type: shared LIR value-ID allocation and ownership blocker
Blocks: `ideas/open/778_lir_logical_rhs_result_authority_publication.md`

## Goal

Diagnose and repair the shared cross-function `LirValueId` allocation/ownership
model required for the selected logical RHS standalone cast-result authority
opt-in, then restore full-suite non-regression.

## Why This Exists

778 accepted the selected logical RHS allocation, authority opt-in, and
focused positive/malformed proof in `3b716c12d`, `54ebfa4df`, and `b4685da80`.
Its focused guard passed, but its full-suite baseline candidate is rejected:
3037 total tests, 2883 passed, and 154 failed. The failures chiefly report
foreign ownership, including `pr52129.c`:
`LirCastOp.result: standalone native cast result LirValueId is owned by another
LirFunction`.

The selected cast is correctly using the accepted standalone verifier contract;
the shared native-ID allocation/ownership model is instead producing IDs that
collide with or appear foreign across `LirFunction`s. That model is outside
778's one logical-RHS producer scope and must be repaired independently.

## In Scope

- trace `fresh_value`/native `LirValueId` allocation, ownership registration,
  and standalone cast-result verification across multiple `LirFunction`s;
- make the smallest shared-model repair supported by that evidence so the
  selected opt-in cast receives a valid current-function-owned native ID
  without cross-function collision or false foreign ownership;
- add nearby same-feature multi-function positive and malformed ownership
  coverage that exercises the repaired model, rather than a single named
  external testcase; and
- prove a fresh build, focused guard, and full-suite candidate restore
  non-regression before returning control to 778.

## Out Of Scope

- PHI result/incoming or predecessor/edge work; generic expression API,
  ternary/coerce, vaarg, or other producer-family migration;
- weakening, bypassing, or logical-only special-casing the accepted flag-gated
  standalone cast-result verifier contract;
- Raw-BIR/importer, backend, target lowering, MIR, emission, text recovery,
  maps, side tables, synthetic values, and testcase-specific branches; and
- publishing 778's 775 handoff or closing 778.

## Acceptance Criteria

- Evidence identifies the allocation/ownership path responsible for
  cross-function collisions or false foreign ownership and justifies the
  bounded shared-model repair.
- Selected opt-in casts in distinct `LirFunction`s receive valid,
  current-function-owned native IDs; malformed missing, invalid, duplicate,
  and genuinely foreign IDs continue to fail closed under the existing
  flag-gated verifier contract.
- Nearby multi-function coverage proves the repaired shared behavior without
  PHI/generic migration.
- A fresh build, matching focused guard, and fresh full-suite candidate show
  restored non-regression. The 778 full candidate remains rejected until then.

## Reviewer Reject Signals

- Reject testcase-only exemptions, named-case matchers, expectation downgrades,
  or edits to baseline artifacts claimed as an ownership-model repair.
- Reject weakening or bypassing the flag-gated standalone verifier, including
  allowing absent, invalid, duplicate, or truly foreign authority to pass.
- Reject a logical-only special case that hides the shared cross-function
  allocation/ownership defect instead of repairing the supported model.
- Reject PHI/generic expression migration, other producer-family absorption,
  text-derived recovery, maps/side tables, or broad unrelated rewrites.
- Reject coverage that proves only `pr52129.c` or retains the same foreign-ID
  failure behind renamed helpers or changed diagnostics.

## Parent Return Point

After this blocker is accepted, reactivate 778 at its
baseline-acceptance/Step-4 boundary. Rerun its focused build/test, matching
focused guard, and a fresh full-suite candidate before publishing the bounded
775 handoff.

## Closure Record

Disposition: **close accepted — capability complete.**

The bounded shared allocation/ownership repair was accepted in `7b9d6152b`
(`lir: allocate native values module-wide`) and nearby multi-function authority
proof was accepted in `3602e8fd2` (`test: prove module-wide lir value
ownership`). The focused build and matching guard passed:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_call_type_ref|llvm_gcc_c_torture_src_pr52129_c)$'`

Both selected tests passed (2/2). The supervisor accepted the fresh full-suite
candidate, 3037/3037 passed, and restored `test_baseline.log`; the candidate
was removed. This resolves the rejected 778 baseline without weakening the
flag-gated malformed-authority contract or expanding into PHI/generic work.

778 resumes at its preserved baseline-acceptance/Step-4 boundary. Before its
handoff, it must rerun its focused build/test, the matching focused guard, and
a fresh full-suite candidate. Its scope remains logical RHS only.
