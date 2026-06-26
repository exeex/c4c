Status: Active
Source Idea Path: ideas/open/377_rv64_object_route_instruction_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Residual Instruction-Fragment Audit

# Current Packet

## Just Finished

Step 4 follow-up / Step 5 preparation: Residual Instruction-Fragment Audit
completed for the single `src/20000217-1.c` RV64 gcc-torture backend object
representative after scalar integer call-result stack-slot publication support.

A temporary diagnostic-only formatter at the generic object-route rejection site
identified the precise residual, then the formatter was removed and `c4cll` was
rebuilt back from source. The representative still stops at:

```text
unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering
```

The residual is not a distinct downstream owner. It is still `main`'s ordinary
same-module `bir.call i16 showbug(ptr %lv.x, ptr %lv.y)`, with both pointer
arguments accepted as local-frame-address materializations into `a0`/`a1`.
The result publication is the i16 call result from ABI register `a0` to `%t2`'s
stack-slot home, slot `#12` at stack offset `4`, size `2`, with late
source-register publication available.

Rejection site: `fragment_for_prepared_call()` reaches the stack-slot
call-result publication branch but returns `nullopt` because the prepared result
plan reports `value_bank=None` and `source_register_bank=None`, while the branch
currently requires both to be `Gpr`. The failure then falls through to the
generic `unsupported_instruction_fragment` diagnostic.

The prior Step 1 call-result blocker is therefore not gone in this checkout; it
remains in scope for idea 377 as a narrower scalar integer stack-slot
call-result publication variant, not a lifecycle handoff.

## Suggested Next

Run a focused implementation packet in `fragment_for_prepared_call()` for scalar
integer stack-slot call-result publication when prepared late-publication facts,
the ABI source register, the BIR result type, and the destination stack home are
sufficient even though the result plan's prepared banks are `None`.

## Watchouts

- The admitted lowering is metadata-driven; it does not key on function names,
  value ids, frame-slot ids, concrete registers beyond prepared ABI/source
  metadata, instruction indexes, source syntax, or prepared-BIR text.
- Preserve fail-closed behavior for pointer results, non-scalar sizes,
  missing source register/name, mismatched destination slot/offset, FPR/VREG
  cases, multi-lane widths, and unsupported destination homes.
- Do not close or hand off idea 377 on the current evidence; this is still the
  audited call-result publication family.

## Proof

Audit-only packet. No root-level logs were written. A temporary diagnostic probe
required rebuilding `c4cll`; after removing the probe, `c4cll` was rebuilt back
from source:

```sh
cmake --build build --target c4cll
```

Diagnostic capture commands wrote only delegated `build/agent_state/` artifacts:

```sh
build/c4cll -I . --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20000217-1.c > build/agent_state/377_step4_residual_20000217-1.prepared-bir.txt 2> build/agent_state/377_step4_residual_20000217-1.prepared-bir.err
build/c4cll -I . --codegen obj --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20000217-1.c -o build/agent_state/377_step4_residual_20000217-1.o > build/agent_state/377_step4_residual_20000217-1.codegen-obj.log 2>&1
```

Result: object-route compile exited `2`, as expected for the residual audit.

Artifact paths:

- `build/agent_state/377_step4_residual_instruction_fragment_audit.txt`
- `build/agent_state/377_step4_residual_20000217-1.codegen-obj.log`
- `build/agent_state/377_step4_residual_20000217-1.codegen-obj.rc`
- `build/agent_state/377_step4_residual_20000217-1.prepared-bir.txt`
- `build/agent_state/377_step4_residual_20000217-1.prepared-bir.err`
