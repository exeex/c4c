Status: Active
Source Idea Path: ideas/open/797_lir_to_new_bir_final_coverage_convergence.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair only matrix-proven dispatcher and verifier gaps

# Current Packet

## Just Finished

Completed plan Step 2 audit packet by comparing the terminal matrix against
Raw-BIR importer dispatch, typed destination containers, and reachable
verifier behavior for the matrix-authorized rows.

No code repair is authorized in this packet. The only post-matrix accepted
typed receiver row, closed 867/734 Step 7.52's selected direct-local
`LirVaStartOp` destination `va_list` pointer authority, is already dispatched
by `src/backend/bir/lir_to_bir.cpp` into `VaStartAuthorityNode`, stored in the
typed Raw-BIR container/view/builder surface, verified by
`src/backend/bir/verify/verifier.cpp`, and covered by
`test_selected_direct_local_va_start_authority_receipt_and_rejections()` in
`tests/backend/bir/backend_lir_to_bir_interface_test.cpp`.

The remaining matrix rows are accepted no-change/evidence dispositions or
preserved open scopes, so Step 2 must not fabricate new receiver rows for
compact scalar/ABI-leaf, first-class boundary, aggregate/global/carrier,
global policy, intrinsic/inline-assembly evidence, body-parameter, residual
instruction/terminator, switch-selector, or opaque/render text families.

## Suggested Next

Supervisor should review and, if accepted, commit the Step 2 no-op audit
record, then dispatch Step 3 from `plan.md`: select and run the narrow
whole-module transactional proof for the accepted disposition set.

## Watchouts

- 813's closed 848/849/850 dispositions are evidence/no-change inputs and do
  not fabricate direct 734 receiver work.
- 847 is terminal deletion evidence for 797 only; it does not prove 797
  complete.
- Preserve open 795, 796, 821, and 822 scopes. Do not absorb body-parameter,
  residual instruction/terminator, inline-assembly, or switch-selector routes
  into this 797 packet.
- Do not reopen accepted 734 Step 7.52 or the closed 867 producer handoff.
- Reject text, `monostate`, rendered operand, printer output, testcase
  identity, or compatibility-mirror authority.
- `src/backend/bir/lir_to_bir/README.md` still has older planning rows saying
  generic `va_start` semantic payload wiring is missing; Step 2 did not edit
  that broader planning note because the matrix-authorized row is narrower:
  selected direct-local `VaStartAuthorityNode` receipt only.

## Proof

No-op audit proof:

```sh
find docs/lir_to_new_bir_final_coverage_convergence -maxdepth 1 -type f -printf '%f\n' | sort
rg -n "VaStartAuthority|LirVaStartOp|test_selected_direct_local_va_start_authority_receipt_and_rejections" src/backend/bir tests/backend/bir/backend_lir_to_bir_interface_test.cpp
git diff --check
```

Proof output is recorded in `test_after.log`. No build/test command was run
because Step 2 made no code or documentation matrix repair.
