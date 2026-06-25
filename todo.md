Status: Active
Source Idea Path: ideas/open/358_rv64_object_route_abi_width_edges.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Route Simple Call and FPR ABI Edges

# Current Packet

## Just Finished

Completed Plan Step 4 classification for the next `src/20030125-1.c` RV64
object-route boundary after simple prepared FPR direct-call lowering.

Fresh representative proof now moves past the direct-call FPR argument/result
edge and fails at generic prepared move-bundle emission:
`unsupported_move_bundle_target_shape: prepared move bundle requires unsupported
RV64 moves`.

The first object traversal move-bundle event is in function `@t`, block 0,
`before_return`, instruction index 3. The prepared dump records:
`move from_value_id=3 to_value_id=3 destination_kind=function_return_abi
destination_storage=register op_kind=move uses_cycle_temp_source=no
placement=fpr:call_result#0/w1 reg=fa0 reason=return_register_to_register`.
`value_id=3` is `%t2`, home `register bank=fpr reg=ft0`, so the unsupported
semantic move shape is an FPR return ABI move from prepared value home `ft0` to
the function-return ABI FPR register `fa0` (`ft0 -> fa0`). This is not the
previous direct-call `ft0 -> fa0` argument edge; it is a generic
`before_return` move bundle consumed by the object traversal after the
instructions in `@t`.

## Suggested Next

Implement RV64 object-route support for simple prepared FPR `before_return`
register-to-register ABI moves, beginning with the `@t` `ft0 -> fa0` shape at
block 0 instruction 3, while keeping non-F32/F64, non-register, cycle-temp, and
multi-width cases fail-closed.

## Watchouts

- Treat `src/20030125-1.c` as a representative only. The semantic class is
  prepared FPR cast/call ABI lowering, not testcase-specific math folding.
- Admission is intentionally identity-backed for FPR homes. Do not add raw
  `register_name` fallback parsing for FPR spellings in object emission.
- Generic prepared object traversal only emits block-entry and before-return
  move-bundle events. The prepared `before_instruction` FPR conversion bundles
  at instruction indexes 0 and 2 are visible in the dump, but they are not this
  reported `unsupported_move_bundle_target_shape` boundary.
- Existing direct-call FPR lowering already handles call argument/result bundles
  through the call instruction path. Do not route this return-ABI boundary by
  weakening call-plan checks or expectation text.

## Proof

```sh
tmp=$(mktemp); printf 'src/20030125-1.c\n' > "$tmp"; ALLOWLIST="$tmp" CASE_TIMEOUT_SEC=20 STOP_ON_FAILURE=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > test_after.log; rc=$?; rm -f "$tmp"; exit $rc
```

Result: failed as expected for classification. Proof log: `test_after.log`.
Supporting dump evidence: `build/rv64_gcc_c_torture_backend/src_20030125-1.c/prepared.dump`.
