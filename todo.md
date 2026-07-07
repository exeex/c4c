Status: Active
Source Idea Path: ideas/open/564_bir_scalar_local_memory_semantic_admission_followup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Lock Down Current Scalar/Local-Memory Diagnostics

# Current Packet

## Just Finished

Step 1 - Lock Down Current Scalar/Local-Memory Diagnostics completed as a
diagnostic-only packet. The four target IEEE rows all reproduce the same
backend-route admission failure:

`error: RISC-V backend assembly route requires semantic lir_to_bir lowering before the prepared-module handoff: semantic lir_to_bir failed outside the currently admitted capability buckets ... latest function failure: semantic lir_to_bir function 'test_isunordered' failed in scalar/local-memory semantic family`

Row-to-boundary mapping:

- `src/ieee/fp-cmp-8.c`: `test_isunordered` lowers to `%t0 = fcmp uno double %p.x, %p.y`, then `zext i1 %t0 to i32`, `icmp ne i32 %t1, 0`, branch, double phi, return. The failing producer boundary is `BirFunctionLowerer::lower_scalar_or_local_memory_inst()` handling the first `LirCmpOp`; it dispatches directly to `lower_scalar_family_inst()` / `lower_scalar_compare_inst()`, where `lower_cmp_predicate("uno")` returns no BIR `BinaryOpcode`. No BIR compare instruction or compare fact is produced, so `lower_block_insts()` records the umbrella `scalar/local-memory semantic family`.
- `src/ieee/fp-cmp-8f.c`: `test_isunordered` first has scalar casts `%t0 = fpext float %p.x to double` and `%t1 = fpext float %p.y to double`, then `%t2 = fcmp uno double %t0, %t1`, `zext`, `icmp ne`, branch, float phi, return. The `fpext` scalar-cast producers are not the current blocker; the fail-closed boundary is the same `LirCmpOp` unordered floating compare after those casts, with no `uno` BIR comparison opcode/fact.
- `src/ieee/fp-cmp-8l.c`: `test_isunordered` lowers to `%t0 = fcmp uno fp128 %p.x, %p.y`, then `zext`, `icmp ne`, branch, fp128 phi, return. The blocker is the same scalar compare producer boundary for unordered `fcmp uno`, now with `operand_type = F128`; type lowering accepts fp128, but predicate lowering rejects `uno` fail-closed.
- `src/ieee/pr38016.c`: includes `fp-cmp-8.c` and reproduces the same double-shaped `test_isunordered` stream as `fp-cmp-8.c`; it maps to the same `LirCmpOp` unordered floating compare fail-closed boundary.

Owner-boundary check: this is not the local-memory alloca/gep/load/store path,
not scalar-control-flow terminator/phi ownership, and not scalar-binop/cast
ownership after the compare. The current missing semantic fact is a BIR
representation or deliberate fail-closed contract for unordered floating
comparison predicates (`uno`, and likely the nearby `ord`/unordered IEEE
predicate family) at the scalar compare producer boundary.

## Suggested Next

Execute Step 2 from `plan.md`: add focused semantic admission coverage for the
unordered floating compare scalar producer boundary before repairing lowering.

## Watchouts

- Keep scalar-control-flow, function-signature, scalar-binop, and RV64
  object-lowering work out of this runbook unless focused diagnostics prove
  the owner boundary has changed.
- Do not use expectation rewrites, unsupported markers, allowlist edits,
  row reclassification, or named IEEE-row shortcuts as progress.
- The next packet should cover the semantic shape, not only the IEEE filenames:
  `fcmp uno` on accepted floating operand types should either publish a BIR
  compare/condition fact or fail with a narrower intentional diagnostic.
- Nearby functions in `fp-cmp-8.c` include ordered predicates already mapped by
  `lower_cmp_predicate()` (`olt`, `ole`, `ogt`, `oge`, `one`, `oeq`); the
  first live blocker is `uno` because it appears in `test_isunordered`.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: pass. Build reported `ninja: no work to do`; CTest reported `100%
tests passed, 0 tests failed out of 346`.

Proof log: `test_after.log`.
