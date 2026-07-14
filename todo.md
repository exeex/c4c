# Current Packet

Status: Packet Complete - Supervisor Proof Pending
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 7.21
Current Step Title: Publish scalar inline-asm output binding authority

## Just Finished

- Completed Plan Step 7.21 for only PS's scalar integer output-only
  `LirInlineAsmOp` semantic binding and its later Store use.
- Allocated the output through `fresh_value`, kept the compatibility result
  authority-free, stored the exact ID in the native type/Output-role/index-zero
  binding, and preserved it through representation-preserving coercion into the
  later type-matched Store.
- Collected semantic output bindings as function-owned definitions while
  leaving read/write and all other inline-asm routes compatible. Added focused
  source-chain, misleading-display, invalid/duplicate/missing binding,
  role/type/index, unknown/cross-function use, and final-use type rejections.

## Suggested Next

- Supervisor: run broader proof, review the packet diff, and choose the next
  bounded Step-7 row through plan-owner.

## Watchouts

- Own only one output-only scalar integer binding. Its semantic result is a
  definition; the compatibility result remains presentation-only.
- The compatibility `result`, rendered operands, original assembly and
  constraint text, clobbers, and all other rendered mirrors remain
  presentation/opaque. They may observe native authority but never create,
  repair, select, or validate it.
- Preserve the exact binding-to-Store ID and type edge. Read/write scalar
  bindings retain compatibility authority and must not be silently promoted.
- Exclude input, tied/read-write, memory, address, immediate, clobber, explicit
  register, vector, and multi-output bindings; `insn_r` semantics; opaque-text
  interpretation; compatibility-result authority; stack/local/object and body
  parameter publication; CFG; BIR receipt; and all idea-741 contracts.

## Proof

- Fresh `cmake --build --preset default` passed for the Step-7.21 production
  and verifier changes.
- `ctest --test-dir build -R '^frontend_hir_tests$' --output-on-failure`
  passed 1/1 with the focused source-chain, display, and malformed coverage.
- Matched focused `test_before.log` / `test_after.log` passed 1/1 before and
  after with delta 0 passed / 0 failed and no new failures.
- The supervisor's canonical matched full regression guard passed 3033/3033
  before and after, with delta 0 passed / 0 failed and no new failures.
- `git diff --check` passed. The final commit remains supervisor-owned.
