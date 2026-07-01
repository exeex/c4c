Status: Active
Source Idea Path: ideas/open/512_stack_passed_parameter_home_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Rebuild Stack-Parameter Evidence

# Current Packet

## Just Finished

Step 1 of `plan.md` rebuilt stack-parameter evidence for
`tests/c/external/gcc_torture/src/20001017-1.c` at current `HEAD`.

Fresh artifacts live under
`build/agent_state/512_step1_stack_parameter_evidence/20001017-1/`.
The old idea-424 handoff still describes the underlying producer gap, but the
surface object diagnostic has changed:

- Old 424 object diagnostic:
  `unsupported_param_home: RV64 object route requires all parameters in supported GPR or prepared FPR register homes`.
- Current object diagnostic:
  `unsupported_stack_frame: RV64 object route does not support non-GPR prepared callee-saved register save slots (fpr:fs1)`.

The current prepared dump still exposes the stack-parameter-home gap beneath
that earlier FPR save-slot rejection:

- Callee `@bug` parameters 0-7 publish register homes:
  `%p.Cref=a0`, `%p.transb=a1`, `%p.m=a2`, `%p.n=a3`, `%p.k=a4`,
  `%p.a=a5`, `%p.A=a6`, `%p.fdA=a7`.
- Callee stack-passed parameters remain missing homes:
  `%p.B value_id=8 kind=none`, `%p.fdB value_id=9 kind=none`,
  `%p.b value_id=10 kind=none`, `%p.C value_id=11 kind=none`, and
  `%p.fdC value_id=12 kind=none`.
- Stack-layout already has regalloc spill-slot objects and offsets for some
  of those values:
  `%p.B` object `#9` slot `#9` offset `32` size `8` align `8`,
  `%p.fdB` object `#10` slot `#10` offset `40` size `4` align `4`, and
  `%p.fdC` object `#11` slot `#11` offset `44` size `4` align `4`.
  `%p.b` and `%p.C` have no matching value-home object in this dump.
- Caller `@main` call-plan arguments 8-12 all print `to=none`; value-location
  ABI bindings publish `destination_storage=stack_slot abi_index=8..12` but
  do not publish `stack_offset`.
- Named caller stack arguments 8 and 11 have source-frame authority for the
  local objects (`%lv.B.0` slot `#1` offset `8`, `%lv.C.0` slot `#2` offset
  `16`), but that is source materialization, not call-destination/home
  authority.

Current partial support to preserve:

- `src/backend/mir/riscv/codegen/object_emission.cpp` already has fail-closed
  validation for prepared scalar GPR formal stack-slot homes, byval homes, and
  sret homes when producer facts include kind, slot id, offset, size, align,
  and matching prepared frame-slot/object facts.
- `src/backend/prealloc/regalloc.cpp` already records stack-slot ABI bindings
  when `call_arg_destination_stack_offset_bytes` returns an offset.
- `src/backend/prealloc/regalloc/call_return_abi.cpp` currently computes call
  stack offsets only for x86_64 and AArch64; for RV64 it returns `nullopt`,
  leaving the bindings offsetless.
- `src/backend/prealloc/regalloc/value_homes.cpp` publishes fixed formal
  register homes and special F128/variadic stack homes, but ordinary RV64
  stack-passed fixed formals still fall through to `kind=none` when they are
  not assigned register homes.

## Suggested Next

Execute Step 2: trace the producer ABI/prealloc publication path around
`regalloc/call_return_abi.cpp`, `regalloc.cpp`, `regalloc/call_moves.cpp`, and
`regalloc/value_homes.cpp`. The likely boundary is publishing RV64 stack
argument destination offsets for caller ABI bindings and ordinary fixed-arity
callee `PreparedValueHomeKind::StackSlot` homes from producer ABI/prealloc
facts, while keeping RV64 object emission as a consumer of those facts.

## Watchouts

- Keep the route limited to producer ABI/prealloc publication of explicit
  stack-passed parameter homes.
- Do not infer stack argument homes in RV64 from argument indexes, ABI
  formulas, source call shape, parameter names, or named gcc_torture rows.
- Keep varargs, F128, aggregate ABI, dynamic stack work, broad RV64 call
  lowering, and unrelated ABI repairs out of this plan.
- `20001017-1.c` currently trips `fpr:fs1` save-slot rejection before the old
  `unsupported_param_home` check, so Step 2 should use prepared dumps and
  focused synthetic probes to prove the producer facts instead of treating the
  current object diagnostic as evidence that the param-home gap disappeared.
- RV64 already has consumer validation for prepared formal stack-slot homes;
  the next slice should not add RV64 inference from raw ABI indexes or source
  shape.

## Proof

- `cmake --build build --target c4cll` passed.
- `./build/c4cll --dump-bir --target riscv64-unknown-linux-gnu tests/c/external/gcc_torture/src/20001017-1.c`
  passed; output captured at
  `build/agent_state/512_step1_stack_parameter_evidence/20001017-1/dump-bir.txt`.
- `./build/c4cll --dump-prepared-bir --target riscv64-unknown-linux-gnu tests/c/external/gcc_torture/src/20001017-1.c`
  passed; output captured at
  `build/agent_state/512_step1_stack_parameter_evidence/20001017-1/dump-prepared-bir.txt`.
- `./build/c4cll --codegen obj --target riscv64-unknown-linux-gnu tests/c/external/gcc_torture/src/20001017-1.c -o build/agent_state/512_step1_stack_parameter_evidence/20001017-1/20001017-1.o`
  failed as expected with rc `2` and current `fpr:fs1` stack-frame diagnostic.
- `git diff --check -- todo.md` passed.
