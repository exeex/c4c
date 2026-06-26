Status: Active
Source Idea Path: ideas/open/398_rv64_object_route_stack_frame_and_param_home_edges.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Classify Current Stack Frame And Home Rejections

# Current Packet

## Just Finished

Step 1 refreshed the current 398 stack-frame / parameter-home /
variadic-admission seed bucket. The exact delegated proof completed
successfully. No seed currently reproduces an RV64 object-route
`unsupported_function_admission`, `unsupported_param_home`, unsupported
prepared stack-frame, or callee-saved save-slot diagnostic. The first still
visible owned fact gap is producer-side variadic metadata in the prepared dump
for `va-arg-21.c`.

Representative statuses:

- `930603-1.c`: `c4cll_status=0`, `prepared_status=0`; no failure
  diagnostic. Prepared frame/save-slot facts are present: `@main` has
  `frame_size=12`, `saved_regs=3`, saved `gpr:s1` at `slot#3+stack16`, saved
  `fpr:fs1` at `slot#4+stack24`, and saved `fpr:fs2` at `slot#5+stack32`.
  The callee `@fx` has an FPR parameter home `storage %p.x register bank=fpr
  reg=fa0` / `home %p.x kind=register reg=fa0`.
- `20001017-1.c`: `c4cll_status=0`, `prepared_status=0`; no failure
  diagnostic. Prepared parameter-home facts exist for register-passed
  parameters: `%p.Cref`..`%p.fdA` are in `a0`..`a7`/`fa5` with `bank=gpr` or
  `bank=fpr`, width 1. Stack-passed formals `%p.B`, `%p.fdB`, `%p.b`, `%p.C`,
  and `%p.fdC` still print `encoding=none` / `home kind=none` with known
  `bank=gpr` or `bank=fpr`, and same-module call args 8..12 have
  `dest_bank=none`; this shape is currently admitted by the proof and does not
  raise `unsupported_param_home`.
- `va-arg-21.c`: `c4cll_status=0`, `prepared_status=0`; no failure
  diagnostic. Prepared frame facts show `@doit frame_size=304`,
  `variadic_entry=yes`, saved `gpr:s1`/`gpr:s2`, and `%p.s` in
  `frame_slot bank=gpr slot#16+stack128`. The variadic-entry plan records
  `va_list_layout required=yes size=8 align=8 fields=1`,
  `overflow_area required=yes base_slot=#31 base_stack_offset=232`, incoming
  variadic GPR publications from `a1`..`a7` into that overflow area, and
  `helper kind=va_start`. It also records
  `missing fact=helper_operand_homes.va_start.destination_va_list_address`.
  The prepared call facts for `llvm.va_start.p0` use frame-slot sources at
  stack offsets 136 and 144 with complete GPR call-argument destinations, but
  the helper operand home for the destination `va_list` address is still not
  explicitly published.
- `20000412-2.c`: `c4cll_status=0`, `prepared_status=1`; first diagnostic is
  pre-prepared semantic lowering: function `main` failed in semantic call
  family `direct-call semantic family`.
- `20000706-2.c`: `c4cll_status=0`, `prepared_status=1`; first diagnostic is
  pre-prepared semantic lowering: function `main` failed in semantic call
  family `direct-call semantic family`. The textual output includes aggregate
  parameter alloca/store and a direct call to `bar`, but no prepared 398 facts
  are available.
- `20000717-4.c`: `c4cll_status=0`, `prepared_status=1`; first diagnostic is
  pre-prepared semantic lowering: function `x` failed in `gep local-memory`
  semantic family.
- `20010605-2.c`: `c4cll_status=0`, `prepared_status=1`; first diagnostic is
  pre-prepared semantic lowering: function `main` failed in `store
  local-memory` semantic family, with aggregate floating parameter-home
  alloca/store shapes visible only in the textual output.
- `20011008-3.c`: `c4cll_status=0`, `prepared_status=1`; first diagnostic is
  pre-prepared semantic lowering: function `__db_txnlist_lsnadd` failed in
  `load local-memory` semantic family.

Classification:

- Callee-saved save-slot route: not currently justified from this proof.
  `930603-1.c` has explicit GPR/FPR save slots and compiles.
- Prepared stack-frame route: not currently justified from this proof. The
  prepared representatives that reach the handoff have explicit frame sizes,
  alignments, save slots, and stack slots; no unsupported stack-frame
  diagnostic appears.
- GPR/FPR parameter-home route: not currently justified as a target-side
  failure. `20001017-1.c` still exposes stack-passed formals with
  `home kind=none` / `encoding=none`, but the proof admits them and reports no
  `unsupported_param_home`.
- Variadic route: first justified Step 2 route if 398 continues. The current
  prepared data for `va-arg-21.c` is rich enough to identify a producer-side
  missing fact: `helper_operand_homes.va_start.destination_va_list_address` is
  absent even though the variadic entry plan, overflow area, incoming GPR
  publications, `va_list` layout, and `llvm.va_start.p0` call sites are
  present. Do not fabricate this in RV64 object emission; repair or route the
  producer/helper metadata authority.
- The five nearby same-bucket stale-log seeds do not reach prepared handoff, so
  they belong to earlier semantic lowering buckets until those preconditions
  are repaired.

## Suggested Next

If the supervisor keeps 398 active for implementation, make Step 2 a
producer-side variadic helper metadata packet: publish
`helper_operand_homes.va_start.destination_va_list_address` from the existing
variadic entry / va_start planning authority and prove `va-arg-21.c` still
compiles. Do not start with callee-saved save slots, generic stack-frame
admission, or parameter-home target lowering from this refreshed evidence.

## Watchouts

- Do not fabricate stack-frame, callee-saved, parameter-home, or variadic facts
  in RV64 object emission.
- Do not reimplement the ABI classifier in the object emitter.
- Route producer-side missing facts to lifecycle review instead of absorbing
  them into target lowering.
- Do not use filename-specific branches, expectation rewrites, or allowlist
  filtering as progress.
- `20000412-2.c`, `20000706-2.c`, `20000717-4.c`, `20010605-2.c`, and
  `20011008-3.c` are blocked before prepared handoff in semantic LIR-to-BIR
  buckets; do not classify their current prepared/object route from stale logs.
- `20001017-1.c` stack-passed formals with `encoding=none` are observable, but
  currently non-failing. Treat them as watch evidence, not the first Step 2
  implementation target.

## Proof

Exact command run:

```sh
cmake --build --preset default && mkdir -p build/agent_state/398_step1_frame_home_refresh && for case in 930603-1 20001017-1 va-arg-21 20000412-2 20000706-2 20000717-4 20010605-2 20011008-3; do src="tests/c/external/gcc_torture/src/${case}.c"; (build/c4cll --target riscv64-linux-gnu "$src" > "build/agent_state/398_step1_frame_home_refresh/${case}.out" 2> "build/agent_state/398_step1_frame_home_refresh/${case}.err"; printf 'c4cll_status=%s\n' "$?" > "build/agent_state/398_step1_frame_home_refresh/${case}.status"); (build/c4cll --dump-prepared-bir --target riscv64-linux-gnu "$src" > "build/agent_state/398_step1_frame_home_refresh/${case}.prepared.txt" 2> "build/agent_state/398_step1_frame_home_refresh/${case}.prepared.err"; printf 'prepared_status=%s\n' "$?" > "build/agent_state/398_step1_frame_home_refresh/${case}.prepared.status"); done && for case in 930603-1 20001017-1 va-arg-21 20000412-2 20000706-2 20000717-4 20010605-2 20011008-3; do printf '== %s ==\n' "$case"; cat "build/agent_state/398_step1_frame_home_refresh/${case}.status" "build/agent_state/398_step1_frame_home_refresh/${case}.prepared.status"; rg -n 'unsupported_function_admission|unsupported_param_home|callee-saved GPR save slots|supported prepared stack frame|all parameters in supported GPR or prepared FPR register homes|variadic|va_start|va_list|stack frame|frame_slot|stack_slot|callee_saved|save_slot|param|formal|home|storage %p|encoding=register|bank=none|bank=gpr|bank=fpr|reg=|fpr|gpr|unsupported|error|fatal' "build/agent_state/398_step1_frame_home_refresh/${case}.out" "build/agent_state/398_step1_frame_home_refresh/${case}.err" "build/agent_state/398_step1_frame_home_refresh/${case}.prepared.txt" "build/agent_state/398_step1_frame_home_refresh/${case}.prepared.err" || true; done && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Result:

- Command completed successfully.
- `cmake --build --preset default`: no work to do.
- `930603-1.c`, `20001017-1.c`, and `va-arg-21.c`: `c4cll_status=0`,
  `prepared_status=0`.
- `20000412-2.c`, `20000706-2.c`, `20000717-4.c`, `20010605-2.c`, and
  `20011008-3.c`: `c4cll_status=0`, `prepared_status=1` with the semantic
  LIR-to-BIR pre-handoff diagnostics recorded above.
- No `unsupported_function_admission`, `unsupported_param_home`, unsupported
  prepared stack-frame, or callee-saved save-slot diagnostic appeared.
- `test_after.log`: `100% tests passed, 0 tests failed out of 326`.
