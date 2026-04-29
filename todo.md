Status: Active
Source Idea Path: ideas/open/132_parser_rendered_record_template_lookup_mirror_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Make Template Primary And Specialization Lookup Structured-Primary

# Current Packet

## Just Finished

Step 3 added explicit rendered-mirror visibility for template primary and
specialization lookup helpers. `find_template_struct_primary` and
`find_template_struct_specializations` still return structured
`QualifiedNameKey` results, including null structured misses, before rendered
mirrors; stale/mismatched mirrors, matching compatibility mirrors, and
TextId-less rendered fallback use now increment local counters.

## Suggested Next

Next coherent packet: continue Step 3 by tracing remaining
`template_struct_defs` and `template_struct_specializations` consumers outside
the primary/specialization find helpers, then either retire dead rendered mirror
reads or mark any remaining reads as explicit compatibility/final-spelling use.

## Watchouts

The new counters are visibility only; they deliberately do not promote
TextId-less fallback into structured failure and do not change the intended
lookup winner. The rendered maps still exist as compatibility mirrors and
registration/final-spelling bridges.

## Proof

Passed:
`cmake --build build --target frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -R '^frontend_parser_tests$' --output-on-failure >> test_after.log 2>&1`

Proof log: `test_after.log`.
