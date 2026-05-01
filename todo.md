# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 3.3
Current Step Title: Remove Remaining Sema Owner/Member/Static Rendered Routes

## Just Finished

Continued Step 3.3 by replacing the remaining
`has_struct_instance_field_name_by_key(const SemaStructuredNameKey&, const
std::string&)` rendered member-name fallback in Sema. Instance-field recovery
now checks structured field `TextId` metadata first, rejects stale structured
member IDs when that metadata exists, and only preserves a separate textless
field-name compatibility path for record fields whose declaration still arrives
without `unqualified_text_id`.

Added focused stale instance-field coverage in
`frontend_parser_lookup_authority_tests`: an implicit method-body reference
whose rendered member spelling is `stale` but whose structured member `TextId`
is `missing` must not recover the real `stale` field after the structured
member-key miss.

## Suggested Next

Continue Step 3.3 by either wiring parser record-field declarations to produce
`unqualified_text_id` consistently or by reviewing the remaining Sema
`lookup_symbol()` rendered compatibility routes, especially enum constants and
local/global cases where the reference still lacks complete qualifier or
namespace producer metadata.

## Watchouts

- The exact remaining instance-field metadata blocker is producer-side:
  parsed record fields in positive cases such as
  `operator_implicit_member_runtime.cpp` still reach Sema with
  `unqualified_text_id == kInvalidText`, while method-body references like `x`
  carry a `TextId`. The new `struct_textless_field_names_by_key_` path is
  compatibility for those textless declarations only; stale structured member
  IDs are rejected when structured field metadata exists.
- `c4c-clang-tool-ccdb` was available on `PATH`, but
  `build/compile_commands.json` did not load `src/frontend/sema/validate.cpp`;
  targeted source reads were used after that tooling miss.
- The static-member template-instantiation optimistic route noted in the prior
  packet remains unchanged.

## Proof

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_' >> test_after.log 2>&1`

Result: passed. `test_after.log` contains a fresh successful build plus 884/884
passing delegated positive Sema tests.

Additional check: `cmake --build --preset default --target frontend_parser_lookup_authority_tests && build/tests/frontend/frontend_parser_lookup_authority_tests`

Result: passed, including the new stale instance-field member lookup authority
coverage.
