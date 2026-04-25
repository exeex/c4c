Status: Active
Source Idea Path: ideas/open/104_hir_safe_legacy_lookup_demotion.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Route Direct Method And Member Probes Through Helpers

# Current Packet

## Just Finished

Completed `plan.md` Step 3 by routing direct operator-expression struct method
probes in `src/frontend/hir/impl/expr/operator.cpp` through the existing helper
APIs.

The `operator_arrow` chaining loop now uses `find_struct_method_mangled` for
method lookup and `find_struct_method_return_type` for rendered return-type
fallbacks, while preserving the existing `find_function_by_name_legacy`
return-type lookup first. The chain keeps const-first method lookup only when
the receiver result type is const, and keeps base-before-const return metadata
fallback order.

`maybe_bool_convert` now uses `find_struct_method_mangled` for `operator_bool`
lookup while preserving the old base-before-const preference.

## Suggested Next

Supervisor should choose the next packet. This slice leaves helper authority and
the rendered maps unchanged; it only makes the operator-expression paths
exercise the parity-recording helper APIs.

## Watchouts

- `src/frontend/hir/impl/expr/operator.cpp` no longer directly probes
  `struct_methods_` or `struct_method_ret_types_`.
- The `operator_arrow` method helper call intentionally passes `rts.is_const`
  to preserve const-object lookup preference.
- The `operator_arrow` return-type helper call intentionally passes `false` to
  preserve the prior base-return-metadata-first lookup order.
- The `operator_bool` helper call intentionally passes `false` to preserve the
  prior base-method-first lookup order.

## Proof

Passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'cpp_(positive_sema_operator_(arrow|bool)|hir_expr_operator_member_helper)' > test_after.log`

Proof log: `test_after.log`.
