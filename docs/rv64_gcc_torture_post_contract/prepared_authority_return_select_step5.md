# Prepared Authority Step 5: Return ABI And Select Publication

Source plan: `ideas/open/552_prepared_move_bundle_target_shape_authority_gaps.md`

Step 5 inspected the three small-family rows from the Step 1 queue:

| Row | Step 1 family | Step 5 result |
| --- | --- | --- |
| `src/20001130-2.c` | `prepared_return_abi_destination_home_authority` | Reroute to RV64 return pointer stack-source lowering |
| `src/20080719-1.c` | `prepared_return_abi_destination_home_authority` | Reroute to RV64 return pointer stack-source lowering |
| `src/pr58726.c` | `prepared_select_publication_source_home_authority` | Reroute to RV64 I16 select stack-publication lowering |

No implementation file changed in this packet. The prepared/module evidence now
shows explicit ABI or select-publication facts for all three rows, so the
remaining failure is not a safe prepared-authority publication repair.

## Return ABI Rows

The return rows already publish the explicit ABI destination:

- `destination_kind=function_return_abi`
- `destination_storage=register`
- `placement=gpr:call_result#0/w1`
- `reg=a0`
- `reason=return_stack_to_register`

The source homes are concrete pointer stack slots. For `src/20001130-2.c`,
`%t1` is `slot#2`, `offset=0`, `size=8`, `align=8`. For `src/20080719-1.c`,
the affected pointer return values include `@deadfish` at `slot#5`,
`offset=16`, `size=8`, `align=8`, and `%t1` at `slot#10`, `offset=40`,
`size=8`, `align=8`.

The RV64 prepared object route already has a before-return stack-to-register
ABI fragment, but that fragment rejects pointer source types. Publishing a
second destination home for the same value would make prepared authority less
precise: the value home remains the source stack slot, and the ABI register
destination is already represented on the move itself.

Residual owner: RV64 return pointer stack-source lowering.

## Select-Publication Row

`src/pr58726.c` now has concrete source and destination homes for the select
publication:

- source `%t10`, `value_id=4`, `kind=stack_slot`, `slot#3`, `offset=2`,
  `size=2`, `align=2`
- destination `%t11`, `value_id=5`, `kind=stack_slot`, `slot#4`, `offset=4`,
  `size=2`, `align=2`
- publication carrier `select_materialization`
- source producer `cast`
- parallel copy site `predecessor_terminator`

The remaining RV64 intent rejection is caused by policy coverage, not missing
prepared source-home authority. RV64 currently admits existing select
stack-source policies for 4-byte stack-to-stack publications and 4/8-byte
stack-source register destinations, while this row is an I16 stack-to-stack
select publication.

Residual owner: RV64 I16 select stack-publication lowering.

## Proof

```text
cmake --build --preset default
ctest --test-dir build -j --output-on-failure -R '^backend_'
ALLOWLIST=build/agent_state/552_step5_return_select_authority.allowlist scripts/check_progress_rv64_gcc_c_torture_backend.sh
```

Results:

- Build passed.
- Backend CTest passed `345/345`.
- Focused Step 5 proof passed `0/3`; all three rows remain failing, but none
  still lacks prepared return ABI or select-publication source-home authority.

Derived artifacts:

- `build/agent_state/552_step5_return_select_authority.allowlist`
- `build/agent_state/552_step5_return_select_authority/row_status.tsv`
