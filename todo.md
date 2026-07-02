Status: Active
Source Idea Path: ideas/open/536_rv64_object_prepared_module_admission_shell_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prune Or Park Remaining Wrappers

# Current Packet

## Just Finished

Step 3 - Prune Or Park Remaining Wrappers: inspected the prepared-module
admission API and remaining object-route wrappers after the Step 2 extraction.
No genuinely dead forwarding wrapper or declaration was safe to remove.

The newly exposed `make_rv64_prepared_module_rejection`,
`make_rv64_prepared_image_rejection`, and
`admit_rv64_prepared_module_function` declarations remain intentionally parked
as the internal prepared-module boundary: `object_emission.cpp` still needs
them to keep admission and diagnostic construction in `prepared_module_emit.*`
while leaving conversion, text module assembly, prepared data-object assembly,
relocation handling, and ELF writing in the object route.

The public object-route wrappers remain intentionally parked. The diagnostics
entrypoints are still used by backend/tests, the optional-return compatibility
wrappers are still live consumers, and removing them would either break the
public API or leak object assembly/conversion coupling back across the prepared
module boundary.

## Suggested Next

Execute Step 4 from `plan.md`: close-readiness review of the extracted
prepared-module admission shell, including final parked-wrapper notes and
confirmation that the cleanup did not move conversion, text module assembly,
prepared data-object assembly, relocation handling, or ELF writing.

## Watchouts

- `prepared_function_to_object_function`, `fragment_for_prepared_instruction`,
  `build_rv64_text_object_module`, `append_rv64_prepared_data_objects`,
  relocation handling, and ELF writing are intentionally still parked in
  `object_emission.cpp`.
- Keep parallel-copy diagnostics in the prepared-object consumer category path,
  and keep generic admission/module/image failures as plain diagnostic strings.
- `object_emission.hpp` remains the public compatibility surface for prepared
  object result structs and public entrypoints; `prepared_module_emit.hpp`
  remains an internal admission/diagnostic boundary, not a replacement public
  object-emission API.

## Proof

Ran the delegated Step 3 proof command. Result: passed, 5/5 targeted tests
green. Proof log: `test_after.log`.

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(prepared_object_consumer_contract|riscv_object_emission|object_model_records|cli_riscv64_variadic_entry_missing_contract_obj|cli_riscv64_unsupported_global_diagnostic_obj)'
```
