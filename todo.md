# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 7.15
Current Step Title: Publish builtin-ffs zero-comparison/select-condition authority

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

- Execute Step 7.15: publish the builtin-ffs scalar zero-comparison result as
  the exact condition use of its existing `LirSelectOp`.

## Watchouts

- Own only PI's shared i32/i64 equality-to-zero `LirCmpOp` inside
  `emit_builtin_ffs_call`. Allocate its result through `fresh_value` and
  preserve that exact result ID as the existing select condition.
- Retain native integer Eq predicate and exact i32/i64 compared type authority;
  publish the structural zero as `LirIntegerImmediate` while leaving the
  prepared builtin argument honest monostate when it lacks native authority.
- Require unique current-function result ownership and exact condition use;
  reject invalid/duplicate results, unknown or cross-function uses, invalid or
  conflicting predicate/mode/type authority, and malformed zero alternatives.
- Accept misleading comparison/condition displays only after native authority
  is proven. Never infer the argument result or select edge from rendered text.
- Preserve Steps 7.2, 7.3, 7.13, and 7.14. Exclude the cttz call result, other
  builtin comparisons/calls/selects, pointer/vector/aggregate/object work,
  CFG/parameters, inline assembly, and BIR.

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
