# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 7.14
Current Step Title: Publish builtin-ffs plus-one result/select-use authority (complete)

## Just Finished

- Completed Plan Step 7.14 for only the shared i32/i64 builtin-ffs add-one
  producer and its existing select false-arm use.
- Allocated the add-one `LirBinOp.result` through `fresh_value`, retained native
  integer Add and exact i32/i64 type authority, published exact
  `LirIntegerImmediate{1}`, and preserved the result ID as the false arm.
- Kept the internal cttz lhs, zero comparison, and select condition honest
  compatibility; no other builtin, call, binary, or select family was claimed.
- Added exact-ID and misleading-display positives plus invalid/duplicate,
  unknown/cross-function, invalid/conflicting opcode/type, wrong immediate
  authority, and unrepresentable-immediate rejections. The matrix records the
  exact boundary.
- Repaired the full-guard regression exposed by converted integer literals:
  the common ordinary scalar binary seam now withholds immediate authority when
  the payload is not representable by the normalized operation width. This
  preserves legacy modulo/bit-pattern rendering without weakening Step 7.14's
  exact representable immediate-one contract or the verifier invariant.

## Suggested Next

- Select the next bounded Step-7 ordinary-value identity row from the source
  idea and establish its exact producer/use boundary before implementation.

## Watchouts

- Preserve the Step-7.14 contract: only PI's shared scalar i32/i64 add-one is
  newly authoritative, with exact fresh result, native Add/type, immediate one,
  and exact false-arm identity.
- Ordinary scalar binary literals retain authority only when their payload is
  representable by the normalized operation type. Keep converted modulo/
  bit-pattern literals compatibility-only until a richer native payload model
  exists.
- Keep the cttz-produced lhs honest monostate until its call-result row is
  separately owned. Do not infer it or any select edge from rendered text.
- Preserve Steps 7.3 and 7.13 and continue excluding the zero comparison,
  select condition, other builtins/calls/binaries/selects, pointer/vector/
  aggregate/object work, CFG/parameters, inline assembly, and BIR.

## Proof

- Fresh `cmake --build --preset default` passed after the Step-7.14 regression
  repair.
- `ctest --test-dir build -R '^(frontend_lir_call_type_ref|llvm_gcc_c_torture_src_pr68648_c)$' --output-on-failure > test_after.log`
  passed 2/2: the focused authority contract and the exact torture regression.
- Focused coverage now proves converted out-of-range integer literals remain
  compatibility-only while the ffs immediate one remains authoritative and an
  injected unrepresentable authoritative ffs operand still rejects.
- The supervisor's matched full regression guard passed 3033/3033 before and
  after the repair, with delta 0 passed / 0 failed and no new failures.
- `git diff --check` passed for the complete Step-7.14 slice.
