Status: Active
Source Idea Path: ideas/open/104_hir_safe_legacy_lookup_demotion.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Route Direct Method And Member Probes Through Helpers

# Current Packet

## Just Finished

Completed `plan.md` Step 3 by routing direct range-for struct method probes in
`src/frontend/hir/impl/stmt/range_for.cpp` through the existing
`find_struct_method_mangled` helper for `begin`, `end`, `operator_neq`,
`operator_preinc`, and `operator_deref`.

Also routed the range-for `begin` and `operator_deref` rendered return-type
fallback probes through `find_struct_method_return_type` while preserving the
existing legacy `find_function_by_name_legacy` return-type lookup order and the
old base-before-const fallback preference for return metadata.

## Suggested Next

Supervisor should choose the next packet. This slice leaves helper authority and
the rendered maps unchanged; it only makes the range-for path exercise the
parity-recording helper APIs.

## Watchouts

- `lower_range_for_stmt` no longer directly probes `struct_methods_` or
  `struct_method_ret_types_`; broader helper authority changes remain future
  work.
- The helper calls for iterator operators intentionally pass `false` to preserve
  the prior base-method-first lookup order.
- The return-type fallback helper calls intentionally pass `false` to preserve
  the prior base-return-metadata-first lookup order.

## Proof

Passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'cpp_(positive_sema_range_for_|hir_stmt_range_for_helper)' > test_after.log`

Proof log: `test_after.log`.
