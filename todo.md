# Current Packet

Status: Active
Source Idea Path: ideas/open/828_lir_direct_scalar_unary_fneg_authority.md
Source Plan Path: plan.md
Current Step ID: 2R
Current Step Title: Classify and resolve the expanded-baseline regression
你該做code review了


## Just Finished

- Completed former plan Step 2: unary floating-minus now publishes `LirScalarBinaryLhsParameterAuthority` only when its direct `fneg` operand structurally matches one current-function native `DirectScalar` definition by value, exact `LirTypeRef`, owner, and ABI, with role `Lhs`. The verifier now rejects populated `fneg` rhs operands while retaining the existing non-`FNeg` empty-rhs rejection. Nearby coverage proves the published tuple plus omitted-authority, mismatched-tuple, and populated-rhs failures. Commit `524b24f64`; fresh build and focused `^frontend_lir_function_signature_type_ref$` CTest passed, with narrow guard logs 1/1 and no new failures.

## Suggested Next

- Execute plan Step 2R: the hook's expanded baseline comparison now reports a new `frontend_hir_tests` SEGFAULT (0/3038 failures before, 1 failure after). Isolate its relation to `524b24f64` while preserving dirty 821/822/825 work; repair an attributable unary route or route a proven unrelated blocker before any closure/return to 827.

## Watchouts

- Preserve the dirty 821/822/825-related worktree changes. The Step 2 patch intentionally does not touch binary producers, switch selectors, Raw-BIR/importer, schemas, or non-unary-fneg rows.

## Proof

- Existing narrow proof only: build and focused CTest passed as above. Expanded guard: `test_baseline.new.log` reports `frontend_hir_tests` SEGFAULT; baseline review rejected, so it is not accepted closure proof.
