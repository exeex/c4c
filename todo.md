Status: Active
Source Idea Path: ideas/open/159_sema_consteval_domain_table_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory rendered consteval authority

# Current Packet

## Just Finished

Completed Step 1, "Inventory rendered consteval authority." Classified the
rendered string-keyed consteval surfaces in `consteval.hpp`/`consteval.cpp`:
`ConstMap`, `TypeBindings`, rendered NTTP maps, `type_binding_*_by_name`
bridges, `ConstEvalEnv::lookup(std::string)`, `lookup_rendered_nttp`,
`bind_consteval_call_env` mirrors, `lookup_consteval_function`, and
`InterpreterBindings::by_name`. Added owner/limitation/removal or fallback
comments for retained bridges without changing behavior.

## Suggested Next

Start Step 2 from `plan.md`: make value and interpreter-local lookup
structured/text-first, preserving rendered `ConstMap` and
`InterpreterBindings::by_name` only as documented no-metadata compatibility
fallbacks.

## Watchouts

- `ConstEvalEnv::lookup(std::string)` remains the no-metadata rendered value
  fallback; Node-based lookup must continue to gate metadata-rich misses before
  entering it.
- `lookup_rendered_nttp` is currently a narrow compatibility bridge for
  unqualified NTTPs after other metadata channels were seen; covered NTTP
  metadata misses must not reach it.
- `InterpreterBindings::by_name` is still exposed through `env.local_consts`
  beside `by_text` and `by_key`; Step 2 should verify metadata-rich local
  misses cannot be reopened by rendered spelling.
- Do not weaken tests, mark supported paths unsupported, or rely on
  testcase-shaped shortcuts.

## Proof

Ran delegated proof:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^positive_sema_'; } > test_after.log 2>&1`

Result: passed. `test_after.log` contains the successful build and 34/34
passing `positive_sema_` tests.
