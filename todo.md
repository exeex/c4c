Status: Active
Source Idea Path: ideas/open/374_rv64_object_route_non_register_param_homes.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Prepared Non-Register Parameter Homes

# Current Packet

## Just Finished

Step 1 audited the prepared non-register parameter-home facts for
`tests/c/external/gcc_torture/src/va-arg-13.c` at the current
`unsupported_param_home` RV64 object-route boundary.

First boundary shape:

- Function: `test(int fmt, ...)`.
- Parameter: named scalar `%p.fmt`, prepared value id `6`, BIR type `i32`.
- Home kind: `stack_slot`, not a register.
- Storage plan: `encoding=frame_slot bank=gpr spill_slot=slot#11+stack64 width=1
  slot_id=#11 stack_offset=64`.
- Value home: `home %p.fmt value_id=6 kind=stack_slot slot_id=11 offset=64`.
- Stack object: object `#9`, function `test`, name `%p.fmt`,
  `source_kind=regalloc.spill_slot`, type `i32`, size `4`, align `4`,
  `address_exposed=no`, `requires_home_slot=no`, `permanent_home_slot=no`.
- Frame slot: slot `#11`, object `#9`, function `test`, offset `64`, size `4`,
  align `4`, fixed location `no`.
- Final prepared frame plan: `@test frame_size=80 frame_alignment=8
  has_dynamic_stack=no fixed_slots_use_fp=no`; slot `#11` is inside that frame.
  The earlier `frame_size=48` shown under prepared addressing is a stale/local
  addressing block value that predates the later regalloc spill slots and RV64
  variadic fixed slots, so it should not be used as the final frame bound.
- Address/materialization facts for `%p.fmt`: none needed and none exposed for
  the current formal setup shape; it is a scalar parameter value spilled from
  the incoming GPR argument register into the prepared frame slot.
- ABI ownership: caller call plan for `main -> test` passes arg0 immediate
  `456` to `a0` and arg1 immediate `1234` to `a1`; `%p.fmt` is the named fixed
  parameter owned by callee `test`, with `named_params=1 named_gp=1 named_fp=0`.
- Variadic relation: RV64 variadic entry plan for `test` has overflow area
  `base_slot=#12 base_stack_offset=72 align=8`; the stack-slot `%p.fmt` at
  offset `64` is immediately before the overflow area base at `72`, matching the
  fixed-parameter home that `va_start` needs as the last named parameter
  boundary.
- Call-plan relation: no call-argument move is required for `%p.fmt` itself.
  The later `dummy` calls expose separate frame-slot-address argument facts for
  `%t7` and `%t14`; those are adjacent but not the first
  `unsupported_param_home` boundary.

Conclusion: this first non-register parameter-home shape is supportable. The
actual scalar lowering surface is RV64 formal-entry setup:
`append_riscv_formal_entry_homes` can store a supported GPR scalar formal from
incoming `a0` to `home->offset_bytes` when the prepared home is a stack slot.
The explicit missing fact is not in the prepared dump for `%p.fmt`; the missing
support is in the RV64 object-route admissibility gate, which currently rejects
all non-register non-byval parameter homes before the formal-entry stack-store
path can run.

## Suggested Next

Execute a narrow implementation packet that teaches the RV64 object-route
`unsupported_param_home` check to admit scalar GPR formal stack-slot homes when
the prepared home, frame slot, type, size/alignment, and signed 12-bit stack
offset are coherent, then prove that `append_riscv_formal_entry_homes` reaches
and emits the existing stack-store path for `%p.fmt`.

## Watchouts

- Do not infer parameter layout from source syntax, testcase names, raw BIR
  spelling, or assumed RV64 stack layouts.
- Preserve fail-closed diagnostics for unsupported stack, aggregate,
  missing-home, non-default address-space, dynamic-layout, and ABI-ambiguous
  parameter homes.
- Keep byval aggregate homes, aggregate `va_arg`, and frame-slot address call
  arguments routed to their owning ideas unless the parameter-home contract
  directly requires them.
- Do not weaken unsupported expectations, allowlists, diagnostics, or broad
  test contracts to claim progress.

## Proof

Audit-only packet. Read-only diagnostics used:
`build/c4cll -I tests/c/external/gcc_torture --target riscv64-linux-gnu
--codegen obj tests/c/external/gcc_torture/src/va-arg-13.c -o
/tmp/va-arg-13.audit.o`, which confirmed the current
`unsupported_param_home` diagnostic. No implementation proof was required, and
no canonical `test_after.log` or `test_before.log` was overwritten.
