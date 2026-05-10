Status: Active
Source Idea Path: ideas/open/160_sema_canonical_symbol_template_key_authority.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Keep ABI And Debug Paths Rendering-Only

# Current Packet

## Just Finished

Step 6 confirmed ABI mangling and debug formatting remain rendering-only paths
in `canonical_symbol.cpp`.

`format_canonical_type`, `mangle_type_impl`, and `mangle_name` now carry explicit
comments that they consume display/source spelling and must not become semantic
lookup authority. The focused metadata test proves structured identity equality
can differ while `format_canonical_type`, `mangle_type`, and `mangle_symbol`
still render the same stable display/source text.

## Suggested Next

Supervisor should decide whether Step 6 closes the current runbook slice or
whether another bounded authority site needs review before lifecycle handling.

## Watchouts

- No semantic lookup code changed in this packet.
- ABI/debug rendering is intentionally allowed to collide for semantically
  distinct entities with the same display/source spelling; lookup authority
  remains `CanonicalIdentity`, `types_equal`, and their structured metadata.

## Proof

Ran exactly:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(cpp_hir_sema_canonical_symbol_structured_metadata|cpp_positive_sema_lookup_value_structured_metadata)$'`

Result: passed. Full combined output is preserved in `test_after.log`.
