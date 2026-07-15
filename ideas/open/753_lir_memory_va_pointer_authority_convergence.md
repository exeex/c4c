# LIR Memory And VA Pointer Authority Convergence

Status: Open
Type: bounded LIR memory/va pointer authority repair
Predecessor: `ideas/open/752_lir_local_object_pointer_authority_convergence.md`

## Goal

Move memory intrinsic and va-list operations away from text-only pointer
operands by consuming structured pointer/object/lifetime authority from the
local/object model.

## Why This Exists

`LirMemcpyOp`, `LirMemsetOp`, `LirVaStartOp`, `LirVaEndOp`, `LirVaCopyOp`, and
`LirVaArgOp` still mostly carry monostate/text operands. Selected memcpy work
handles one row, but the broader family will keep reintroducing string-based
semantic gaps unless the common memory/va pointer boundary is closed.

## In Scope

- Generalize from the selected memcpy row only after idea 752 supplies the
  local/object pointer substrate.
- Publish structured pointer/object/lifetime authority for representative
  memcpy, memset, va_start, va_end, va_copy, and va_arg routes.
- Preserve typed size/value operands where applicable and reject malformed
  size, pointer, owner, and lifetime authority.
- Keep unconverted memory/va rows fail-closed or explicitly compatibility-only.
- Add focused positive and negative coverage across memory and va-list
  producers, including at least one vaarg route that emits memcpy-like moves.

## Out Of Scope

- Raw-BIR receiver work, target lowering, MIR, emission, alias analysis, or
  full memory model semantics.
- CFG/PHI work, local/object substrate definition, aggregate/vector carrier
  publication, or opaque inline-asm text.
- Treating builtin names, operand spelling, rendered LLVM, or testcase shape as
  semantic authority.

## Acceptance Criteria

- Representative memory and va-list operations consume structured pointer and
  lifetime authority instead of parsing text.
- The verifier rejects invalid, foreign, type-mismatched, size-mismatched, or
  dead pointer/object authority.
- The selected memcpy route remains a special proven row, not the only memory
  operation with structured authority.
- Full baseline acceptance requires 100% passing tests. If a baseline run is
  below 100%, reject closure and trace `log/*` by time/commit to identify the
  first bad commit before continuing.

## Resumed Carrier Handoff Record

- **Previously accepted 753 progress:** 753 Step 2 accepted `va_start`/`va_end`
  in `8f6f4f9c9`, positive-size aggregate `memset` baseline repair in
  `0912c93a9`, `va_copy` in `17ba5b129`, and AMD64 scalar/pointer `va_arg` in
  `52f143765`. The scalar/pointer packet's backend guard passed 5/5 and its
  accepted full baseline passed 3037/3037.
- **Resolved blocker:** closed idea
  `ideas/closed/799_lir_amd64_vaarg_overflow_aggregate_carrier_authority.md`
  accepted its Step 2 implementation in `c4e820a48`. It supplies exactly one
  selected AMD64 SysV aggregate `layout.needs_memory` overflow memcpy carrier:
  the direct current-function `va_list` local, typed field-2 GEP address,
  overflow-pointer load, `Amd64SysVOverflowArgArea` storage kind, live local
  aggregate temporary, final load, struct payload type, and positive i64 byte
  size. The verifier requires the direct field-2/load chain and canonical
  owner/object/liveness facts; it rejects unselected/partial or mixed carrier
  fields, arbitrary/non-derived sources, foreign or dead locals, destination
  disagreement, and type/size disagreement.
- **Accepted blocker proof:** `c4e820a48`; `cmake --build --preset default &&
  ./build/tests/backend/bir/backend_lir_selected_pointer_authority_test`
  passed. The matching backend baseline is recorded in `test_after.log` (5/5).
- **Exact return point:** resume 753 Step 2, *Publish and verify native
  pointer/object/lifetime authority*, and select only the matching producer
  packet that consumes this checked carrier for the AMD64 aggregate `va_arg`
  memcpy row. Do not republish, generalize, or reconstruct aggregate/vector
  carrier authority; Step 3 remains the later source proof and one receiver
  handoff.
- **Remaining work:** Step 2 is now resolved by the accepted closed-799
  implementation with no additional 753 semantic delta. Complete only 753
  Step 3 proof and its one receiver handoff. Do not bridge the row with text
  recovery, absorb aggregate/vector carrier work, or perform Raw-BIR receipt
  work.

## Step 2 No-Delta Resolution

- **Resolution:** the resumed Step 2 requires no additional 753 implementation
  claim. Closed 799 commit `c4e820a48` already wholly selects the matching LIR
  producer row in `vaarg_amd64.cpp`, publishes
  `requires_native_memory_va_authority` and the checked AMD64 SysV overflow
  aggregate carrier, and has `verify.cpp` consume and verify that contract.
  Its nearby focused test covers the positive and malformed boundaries.
- **Accepted proof:** a fresh `cmake --build --preset default` plus
  `./build/tests/backend/bir/backend_lir_selected_pointer_authority_test`
  passed; the matching `test_after.log` baseline is 5/5, and the supervisor
  regression comparison passed non-decreasing at 5/5.
- **Exact next boundary:** Step 3 owns the source-required full baseline and
  exactly one receiver-ready handoff documenting native fields, guarantees,
  rejected forms, and accepted proof. Raw-BIR is only that later receiver and
  remains out of scope: do not perform Raw-BIR receipt/lowering work here.

## Step 3 Baseline Regression Blocker / Resumption Record

- **Last accepted 753 progress:** Step 1 remains accepted through
  `8f6f4f9c9`, `0912c93a9`, `17ba5b129`, and `52f143765`; Step 2 is accepted
  with no new 753 semantic delta through closed-799 implementation
  `c4e820a48`. The last known full-green acceptance is `52f143765`, whose
  full baseline passed 3037/3037. The accepted focused 799 proof is a fresh
  `cmake --build --preset default` plus
  `./build/tests/backend/bir/backend_lir_selected_pointer_authority_test`,
  with matching `test_after.log` 5/5 and non-decreasing supervisor comparison.
- **Interrupted step:** Step 3, *Prove the bounded producer slice and hand off
  one receiver row*. Its source-required full baseline ran 3036/3037: only
  `frontend_lir_call_type_ref` failed, aborting at
  `LirAllocaOp.result: expected operand kind mismatch for '%t18'; got raw-text`.
- **Blocker classification:** the first code commit after the last full-green
  acceptance is `c4e820a48`; its compatibility path in
  `vaarg_amd64.cpp` constructs the unselected temporary as
  `LirOperand::raw(fresh_tmp(ctx))` while passing typed `LirTypeRef` to
  `LirAllocaOp`. This is an unselected AMD64 `va_arg` alloca compatibility
  regression, outside 753's selected producer/receiver-handoff scope. It is
  owned by separate open blocker
  `ideas/open/800_lir_amd64_vaarg_unselected_alloca_compatibility_regression.md`;
  closed 799 remains closed and is not silently reopened.
- **Exact return point:** after 800 has a fresh build plus
  `./build/tests/frontend/frontend_lir_call_type_ref_test` passing and the
  supervisor has accepted a matching 100%-passing full baseline, resume 753
  Step 3 at the remaining action: document exactly one receiver-ready handoff
  with native fields, guarantees, rejected forms, and accepted proof. Do not
  rerun accepted Steps 1/2, republish carrier authority, or perform Raw-BIR
  receiver implementation.
