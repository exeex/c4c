# LIR Memory And VA Pointer Authority Convergence

Status: Closed
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

## Step 3 Blocker Resolution / Active Resumption Record

- **Resolved blocker disposition:** 800 is capability-complete and closes on
  `18e67ea70`. That commit repairs only the unselected AMD64 overflow `va_arg`
  `LirAllocaOp` result by using `fresh_value(ctx)` rather than a raw-text
  temporary; it does not alter closed 799's selected carrier authority.
- **Accepted proof:** a fresh `cmake --build --preset default` and
  `./build/tests/frontend/frontend_lir_call_type_ref_test` passed. The
  supervisor's matching full `ctest --test-dir build -j --output-on-failure`
  passed 3037/3037 and the regression guard comparison against the accepted
  3037/3037 baseline passed with `--allow-non-decreasing-passed` (0 new
  failures).
- **Active return point:** resume Step 3 only at: document exactly one
  receiver-ready handoff with native fields, guarantees, rejected forms, and
  accepted proof. Steps 1 and 2 remain complete; do not rerun them, republish
  carrier authority, or perform Raw-BIR receiver implementation.

## Step 3 Receiver-Ready Handoff

**Selected receiver: Raw-BIR receipt for the one selected AMD64 SysV aggregate
`layout.needs_memory` overflow `va_arg` memcpy row.** This is a handoff-only
boundary: it authorizes no Raw-BIR receipt, lowering, implementation, or
republishing work in 753.

- **Native fields the receiver may consume:** the checked
  `LirAmd64SysVOverflowAggregateCarrier` carries the direct current-function,
  live `va_list` local pointer/object/owner authority; the typed field-2 GEP
  address; the pointer load from that address; explicit
  `Amd64SysVOverflowArgArea` storage kind; the live destination local
  temporary; the final load identity; the struct payload `LirTypeRef`; and a
  positive typed i64 payload-size immediate. The selected `LirMemcpyOp`
  source, destination, and immediate size are bound to those identities.
- **Guarantees and authority boundary:** this carrier is present only on the
  selected, non-volatile direct-local AMD64 SysV aggregate overflow memcpy
  row. It expresses derived overflow storage rather than claiming that the
  loaded overflow pointer is a local object. The LIR verifier is the gate
  before receiver use: it requires canonical current-function owner/object/
  liveness facts, the direct typed field-2 GEP-to-pointer-load chain, matching
  memcpy source/destination/final-load identities, struct payload type, and
  positive i64 byte-size agreement. It rejects partial or unselected/mixed
  carrier fields, arbitrary or non-derived sources, foreign or dead locals,
  destination disagreement, and type/size disagreement.
- **Rejected forms:** the receiver must not recover pointer, object, lifetime,
  size, storage, or row-selection facts from printed operands, rendered
  LIR/LLVM, names, builtin spelling, or testcase shape. It must not treat an
  unselected compatibility row as receipt-ready, reconstruct a carrier from
  text, or extend this handoff to other aggregates, vectors, targets, scalar
  `va_arg`, or generic memory rows.
- **Accepted proof:** closed-799 implementation `c4e820a48`; a fresh
  `cmake --build --preset default` and
  `./build/tests/backend/bir/backend_lir_selected_pointer_authority_test`
  passed, with the matching backend baseline 5/5. Blocker repair `18e67ea70`
  then passed a fresh `cmake --build --preset default` and
  `./build/tests/frontend/frontend_lir_call_type_ref_test`; the matching full
  `ctest --test-dir build -j --output-on-failure` baseline passed 3037/3037,
  and the supervisor guard found 0 new failures with
  `--allow-non-decreasing-passed`.

## Closure Record

Disposition: capability complete; the bounded native memory/VA producer
authority convergence closes after the Step 3 receiver-ready handoff.

- **Structured producer authority:** accepted 753 Step 1 commits
  `8f6f4f9c9`, `0912c93a9`, `17ba5b129`, and `52f143765` establish the
  representative direct-local `va_start`/`va_end`, positive-size aggregate
  `memset`, `va_copy`, and AMD64 scalar/pointer `va_arg` routes with native
  pointer/object/lifetime and typed size/value authority. The preserved
  verifier boundary rejects malformed size, pointer, owner, lifetime, type,
  and liveness facts rather than recovering them from text.
- **Memcpy-like aggregate row:** closed-799 commit `c4e820a48` supplies the
  one selected AMD64 SysV aggregate-overflow `va_arg` memcpy carrier, its
  checked native fields, and focused verifier coverage. It preserves the
  predecessor's selected memcpy as a special proven row while ensuring it is
  not the only structured memory/VA authority route; unselected rows remain
  compatibility-only rather than semantic authority sources.
- **Receiver handoff:** the preceding Step 3 section contains exactly one
  Raw-BIR receiver-ready handoff, including consumable native fields,
  verifier-backed guarantees, rejected text-recovery forms, and its accepted
  proof. Raw-BIR receipt/lowering remains explicitly out of scope and was not
  implemented by this idea.
- **Acceptance proof:** `c4e820a48` passed a fresh
  `cmake --build --preset default` plus
  `./build/tests/backend/bir/backend_lir_selected_pointer_authority_test`,
  with matching 5/5 backend baseline and non-decreasing guard. Resolved
  blocker 800 commit `18e67ea70` then passed a fresh build and
  `./build/tests/frontend/frontend_lir_call_type_ref_test`; the supervisor's
  matching full `ctest --test-dir build -j --output-on-failure` baseline
  passed 3037/3037, and the regression guard found 0 new failures with
  `--allow-non-decreasing-passed`.
