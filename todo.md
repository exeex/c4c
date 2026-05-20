Status: Active
Source Idea Path: ideas/open/328_aarch64_byval_aggregate_call_argument_lane_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Caller And Callee Byval Placement Agreement

# Current Packet

## Just Finished

Step 2 completed the caller/callee byval slot agreement repair and the
outgoing stack argument base repair for the fixed non-HFA AArch64 byval
transition.

Implemented changes:

- caller byval stack-lane publication now accepts stack-passed byval ABI
  metadata, allows frame-slot byval sources without a source register bank,
  recovers the call argument plan by ABI index for byval lane moves, and
  synthesizes missing stack-lane moves from the prepared call plan
- caller stack-lane publication falls back to byte loads/stores when prepared
  aggregate source bytes are byte-aligned and wider chunks are not printable
- outgoing stack argument destinations now use a distinct outgoing-area base
  (`x16 = sp - outgoing_stack_bytes`) and the call node lowers/restores `sp`
  around the `bl`, so stack argument stores no longer alias caller frame slots
  at `[sp + 0]`
- callee entry formal unpack now counts rounded byval GPR slots, starts
  `%p.a/%p.b/%p.c/%p.d` at `x0/x1/x3/x5`, and forces `%p.e/%p.f` to the
  incoming stack when the two-slot aggregate would cross past `x7`
- callee stack-sourced byval formals now copy aggregate bytes from the
  incoming argument area into the prepared byval home instead of loading a
  pointer-sized scalar
- the x86_64 byval stack-argument carrier index contract was restored
  narrowly so `backend_prepare_frame_stack_call_contract` again sees the
  target ABI carrier destination

Focused dispatch coverage now asserts the rounded byval register-to-stack
formal starts, the caller stack-lane publication, and the distinct outgoing
stack argument base. Generated `arg` now stages `s12` at `[x16 + 0..11]` and
`s13` at `[x16 + 16..28]`, then emits `sub sp, sp, #32; bl fa1; add sp, sp,
#32`. The `fa1` caller/callee placement agrees for `s8..s13`.

The remaining first bad fact from `test_after.log` is outside the original
missing `s12/s13` publication: `c_testsuite_aarch64_backend_src_00204_c`
still fails starting in the later `stdarg:` section. The first visible actual
line is `ABCDEFGHI ABCDEFGHI ABCDEFGHI stdarg:` where the expected line has
six `ABCDEFGHI` fields. The byval `Arguments:` and `Return values:` sections
now match through the repaired fixed aggregate case.

## Suggested Next

Next packet should stay in the later `00204.c` stdarg/HFA/MOVI bucket if the
supervisor wants to continue runtime repair. The fixed byval caller/callee
slot placement and outgoing-area base are now covered by focused tests.

## Watchouts

Do not revert the caller stack stores or the rounded callee unpack as a way to
hide the later `00204.c` runtime mismatch; that would reintroduce the original
byval bug. The outgoing base uses `x16` rather than the reserved MIR scratch
pair because later argument materialization can clobber `x9/x10` before stack
argument stores.

## Proof

`test_after.log` contains the delegated proof:
`git diff --check && cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_aarch64_instruction_dispatch|backend_aarch64_machine_printer|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication|backend_runtime_byval_helper_payload_8_to_13|backend_runtime_byval_helper_payload_9_to_14|c_testsuite_aarch64_backend_src_00204_c)$'`.

Result: `git diff --check` passed and the build passed. The focused CTest
subset is no worse than the accepted known runtime bucket: `backend_prepare_frame_stack_call_contract`,
`backend_aarch64_instruction_dispatch`, `backend_aarch64_machine_printer`,
`backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`,
`backend_runtime_byval_helper_payload_8_to_13`, and
`backend_runtime_byval_helper_payload_9_to_14` passed. The only remaining
failure is `c_testsuite_aarch64_backend_src_00204_c`, starting later in
`stdarg:`.
