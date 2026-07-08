Status: Active
Source Idea Path: ideas/open/611_rv64_terminator_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Broaden Within Authorized Terminator Families

# Current Packet

## Just Finished

Step 3 from `plan.md` implemented RV64 prepared terminator consumer support for
authorized scalar floating fused-compare conditional branches.

The new consumer rule requires a prepared `FusedCompare` branch condition with
an F32/F64 compare type, explicit predicate/lhs/rhs evidence, and materializable
FPR or zero-immediate operands through the existing FP compare helper. It emits
the FP compare into `t3`, branches on the produced GPR predicate, and preserves
the existing false-edge jump. No pointer branch, integer branch, branch-source
publication/freshness, move-bundle, ABI, runtime, expectation, unsupported
marker, or allowlist files were edited.

Focused movement from direct object probes:
- Now compile to RV64 objects: `src/920618-1.c`, `src/pr23941.c`,
  `src/ieee/pr67218.c`, `src/921019-2.c`, `src/20040831-1.c`, and
  `src/ieee/mzero2.c`.
- Moved past `unsupported_terminator_fragment` to downstream owners:
  `src/20000731-1.c` and `src/930603-1.c` to
  `unsupported_scalar_compare_publication`; `src/cmpsf-1.c`,
  `src/ieee/fp-cmp-1.c`, `src/ieee/fp-cmp-6.c`, and
  `src/ieee/pr28634.c` to `unsupported_global_data`; `src/pr35456.c` to
  `unsupported_floating_cast`; `src/ieee/mzero5.c`, `src/ieee/920810-1.c`,
  `src/930702-1.c`, and `src/pr44683.c` to
  `unsupported_instruction_fragment`; `src/990127-2.c` and `src/930614-1.c`
  to `unsupported_move_bundle_target_shape`; `src/921113-1.c` to
  `unsupported_call_abi`; and `src/ieee/pr84235.c` to ambiguous
  non-parallel stack-destination fan-in authority.
- Still report `unsupported_terminator_fragment` after this packet:
  `src/20030910-1.c`, `src/ieee/20001122-1.c`, `src/921124-1.c`,
  `src/920710-1.c`, and `src/991030-1.c`.

Focused out-of-scope checks:
- Pointer compare row `src/20000801-2.c` remains fail-closed at
  `unsupported_terminator_fragment`.
- Branch authority row `src/20000314-3.c` remains fail-closed at
  `unsupported_branch_stack_load_authority`.
- ABI row `src/950607-2.c` remains fail-closed at `unsupported_call_abi`.
- Generic instruction row `src/pr20527-1.c` remains fail-closed at
  `unsupported_instruction_fragment`.

## Suggested Next

Step 3 residual-classification packet:

Refresh direct probes over the remaining current `unsupported_terminator_fragment`
rows and split `src/20030910-1.c`, `src/ieee/20001122-1.c`,
`src/921124-1.c`, `src/920710-1.c`, and `src/991030-1.c` by missing prepared
terminator evidence versus unsupported consumer shape. Keep this diagnostic-only
unless a single semantic family with explicit prepared operands/control-flow
authority emerges.

## Watchouts

- `src/980604-1.c` now compiles as incidental movement through the existing
  integer branch/select path plus the new final floating branch consumer; do
  not use it as the proof target for this floating packet.
- Keep pointer compare branches out of any further floating compare packet.
- Keep branch operand authority, branch stack-source freshness, ABI, malformed
  join evidence, move-bundle, and generic instruction-fragment rows out of
  Step 3 consumer broadening unless a separate plan explicitly owns them.
- Keep `src/20000314-3.c`, `src/20001017-1.c`, `src/20050125-1.c`,
  `src/20080519-1.c`, `src/20140828-1.c`, `src/loop-2e.c`,
  `src/pr39100.c`, `src/20060910-1.c`, `src/930930-1.c`, and
  `src/990127-1.c` as negative authority/freshness rows.
- Keep `src/980604-1.c`, `src/pr58574.c`, and `src/pr88714.c` as mixed
  branch/operand-authority watchouts, not as floating-compare proof targets.
- The direct probe path is `build/c4cll`, not `build/src/apps/c4cll`.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Result: passed. Test subset: `^backend_`. Proof log: `test_after.log`.

Focused diagnostic probes used direct `build/c4cll --codegen obj --target
riscv64-linux-gnu -o <tmp>.o` commands against the GCC torture source paths
under `tests/c/external/gcc_torture/`; temporary objects and probe files were
removed.
