Status: Active
Source Idea Path: ideas/open/753_lir_memory_va_pointer_authority_convergence.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove the bounded producer slice and hand off one receiver row

# Current Packet

## Just Finished

- Step 2 is satisfied with no new 753 semantic delta: accepted 799 commit
  `c4e820a48` already selects the one AMD64 aggregate-overflow row, publishes
  its checked carrier, and verifies it. The focused fresh build plus
  `./build/tests/backend/bir/backend_lir_selected_pointer_authority_test`
  passed; the matching `test_after.log` baseline is 5/5, and the supervisor's
  regression comparison is non-decreasing at 5/5.

## Suggested Next

- Execute Step 3 only: retain the accepted producer proof and document exactly
  one receiver-ready handoff with native fields, guarantees, rejected forms,
  and proof. Raw-BIR receiver implementation remains out of scope.

## Watchouts

- Do not republish or generalize aggregate/vector carrier authority. Do not
  add Raw-BIR receipt work. The accepted gate remains AMD64 SysV + aggregate +
  positive `layout.needs_memory` + checked direct-local `va_list`; other
  targets and indirect/nonlocal routes remain compatibility-only.

## Proof

- Accepted evidence: `c4e820a48`; fresh `cmake --build --preset default` plus
  `./build/tests/backend/bir/backend_lir_selected_pointer_authority_test`
  passed; matching `test_after.log` baseline 5/5 and supervisor comparison
  passed non-decreasing 5/5. Step 3 still owns the source-required full
  baseline and the one receiver handoff record.
