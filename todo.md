Status: Active
Source Idea Path: ideas/open/654_direct_global_stack_backed_pointer_branch_boundary.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Consume Direct-Global Branch Authority In RV64

# Current Packet

## Just Finished

Step 1 of `plan.md`: refreshed the `src/20000314-3.c` direct-global
stack-backed pointer branch evidence without implementation changes.

Fresh probes:
- `./build/c4cll --dump-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20000314-3.c`
- `./build/c4cll --dump-prepared-bir --target riscv64-linux-gnu --mir-focus-function attr_rtx tests/c/external/gcc_torture/src/20000314-3.c`
- `./build/c4cll --codegen obj --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20000314-3.c -o /tmp/c4c-20000314-3.o`

Evidence:
- Semantic BIR branch: `attr_rtx` entry compares `ne ptr %p.varg0, @arg0`
  and branches on `%t1`.
- Prepared branch operands: `branch_condition entry kind=fused_compare
  condition=%t1 compare=ne ptr %p.varg0, @arg0`; `%p.varg0` is GPR-backed
  (`a0`), `@arg0` is stack-backed in `attr_rtx` (`slot #0`, stack offset 0).
- Direct-global identity state: present at the branch producer point as
  `address_materialization block=entry inst_index=0 kind=direct_global
  result=@arg0 symbol=arg0 policy=direct offset=0`.
- Branch stack-load authority state: present and selected for the RHS:
  `branch_stack_load_authority function=attr_rtx block=entry role=rhs
  value=@arg0 ... policy=load_from_stack_slot pointer_status=proven
  status=available source_freshness_status=selected
  source_freshness_authority=branch_stack_slot ... source_freshness_ref_inst=1`.
- Current full-testcase first fail-closed owner changed from the idea-645
  artifact's `unsupported_terminator_fragment` to
  `unsupported_call_abi: RV64 object route requires supported ordinary
  same-module call ABI/result lowering; function=attr_eq; block=entry;
  instruction_index=2; callee=attr_rtx; args=2`.
- Branch-boundary owner if that upstream call ABI gate is bypassed remains RV64
  consumption: producer publication has direct-global identity plus selected
  RHS branch stack-load authority, while the RV64 pointer branch operand mover
  recognizes stack-carried and frame-slot materialized pointer sources before
  generic value movement, but not the explicit direct-global materialization
  shape for this selected RHS stack load.

## Suggested Next

Lifecycle decision: continue the active source idea without rewriting
`plan.md` or creating a separate `ideas/open/` initiative for the newly earlier
`unsupported_call_abi` owner. The call ABI gate is upstream of the representative
full-testcase route and is explicitly out of scope for this source idea, while
the isolated branch-boundary evidence still identifies this idea's missing fact:
RV64 consumption of an explicit direct-global materialization for the selected
RHS stack-backed pointer branch operand.

Treat Step 2 as classified by the Step 1 evidence for the current route:
producer facts are present, including direct-global identity and selected RHS
branch stack-load authority. The next executor should run Step 3 against a
focused isolated/direct branch fixture that reaches `attr_rtx` or an equivalent
direct-global stack-backed pointer branch shape without first passing through
the same-module call ABI blocker.

Only create a separate same-module call ABI idea if the supervisor chooses to
prioritize broad `unsupported_call_abi` recovery independently of this branch
boundary. Do not block this source idea on that separate ABI work.

## Watchouts

- Do not infer direct-global branch authority from stack offsets, source
  spelling, diagnostics, final assembly, or testcase identity.
- Do not reopen `%t6` or `%t23` stack-carried pointer source publication.
- Keep idea 645's closed fused branch family out of scope unless fresh evidence
  proves a regression.
- The current direct-global branch facts are explicit; the first missing branch
  fact appears to be RV64 consumption of direct-global materialization for a
  selected RHS stack-backed pointer branch operand, not producer publication.
- The full `src/20000314-3.c` object route no longer directly proves that
  branch boundary because it stops first at same-module call ABI lowering in
  `attr_eq`.

## Proof

No code changes and no persistent probe artifacts were created, so the delegated
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
proof was not run. No `test_after.log` was produced for this evidence-only
packet.
