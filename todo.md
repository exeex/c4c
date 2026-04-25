Status: Active
Source Idea Path: ideas/open/104_hir_safe_legacy_lookup_demotion.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Route Direct Method And Member Probes Through Helpers

# Current Packet

## Just Finished

Completed `plan.md` Step 3 by routing direct class-specific new/delete struct
method probes in `src/frontend/hir/impl/expr/object.cpp` through
`find_struct_method_mangled`.

`lower_new_expr` now uses the helper for `operator_new` and
`operator_new_array` lookup with `is_const_obj == false`, while preserving the
global operator fallback when no class-specific method is found.

`lower_delete_expr` now uses the helper for `operator_delete` and
`operator_delete_array` lookup with `is_const_obj == false`, while preserving
the global operator fallback when no class-specific method is found.

## Suggested Next

Supervisor should choose the next packet. This slice leaves helper authority and
the rendered maps unchanged; it only makes the object-expression class-specific
new/delete paths exercise the parity-recording helper API.

## Watchouts

- `src/frontend/hir/impl/expr/object.cpp` no longer directly probes
  `struct_methods_`.
- The new/delete helper calls intentionally pass `false` to preserve the
  non-const class-specific lookup route requested by the packet.
- Fallback to the global operator names remains controlled by an empty
  `op_fn`, as before.

## Proof

Passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'cpp_(positive_sema_(class_specific_new_delete|operator_new_delete|new_delete_side_effect)|hir_expr_object_materialization_helper)' > test_after.log`

Proof log: `test_after.log`.
