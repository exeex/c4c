# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 4.5.2
Current Step Title: Reassess remaining function, CFG and local-object authority

## Just Finished

- Completed Plan Step 4.5.1 by receiving exact empty, explicit-void, and
  default-shape nonvariadic `int`, `uint`, `long long`, `unsigned long long`,
  `float`, and `double` declaration/definition signatures.
- Populated existing `FunctionSignature.parameter_types` and builder-created
  typed `ParameterDef` ordinals without source IDs, name maps, or display
  interpretation; Raw and Canonical verification accept the resulting graph.
- Added atomic rejection coverage for missing/reordered/conflicting tracks,
  raw mirrors, metadata/declarator residue, and every excluded neighboring
  parameter family, including I686 `long`/`unsigned long` policy conflicts.

## Suggested Next

- Execute Plan Step 4.5.2 by inventorying the remaining function, CFG,
  stack/local-object, and lifetime rows and selecting one exact structured
  receiver packet or recording its producer blocker.

## Watchouts

- Keep `long` and `unsigned long` fail-closed pending inactive idea 743; do not
  change I686 width semantics inside idea 734.
- Signature `ParameterDef` ordinals are BIR storage order only. Body parameter
  use still lacks native source identity and must not be reconstructed from
  names, raw operands, signature text, or ABI position.
- Production `param_slot.c` now passes signature receipt and rejects later at
  `UnsupportedAllocaInstructions`; pointer parameters still reject at stable
  `UnsupportedFunctionParameters`.

## Proof

- Fresh `cmake --build --preset default` completed successfully.
- `ctest --test-dir build -R '^backend_lir_to_bir_interface$'
  --output-on-failure` passed 1/1.
- `c4cll --dump-bir` published the unused-scalar declaration/definition case;
  the pointer case rejected at `UnsupportedFunctionParameters`, and
  `param_slot.c` rejected later at `UnsupportedAllocaInstructions`.
- `ctest --test-dir build -j --output-on-failure > test_after.log` passed
  3033/3033. The monotonic guard against `test_before.log` passed with delta
  `passed=0 failed=0` and no new over-30-second tests.
