Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce parser lexical scope state for the simplest local bindings

# Current Packet

## Just Finished
Advanced `plan.md` step 2 by teaching the shared visible value/type
classifier to report the post-head token position to statement-side ambiguity
probes as well as constructor-init probes. Local statement disambiguation now
reuses `classify_visible_value_or_type_head()` for qualified visible value
heads instead of reparsing qualified names just to rediscover the token after
the shared lexical/value lookup head, and qualified member-access assignment
forms such as `api::payload.value = 9;` stay on the expression-statement
path.

## Suggested Next
Continue `plan.md` step 2 by auditing the remaining local
declaration/function-declaration ambiguity helpers for unresolved
single-name parameter-style starters that still bypass the shared visible
value/type lookup path, especially the constructor-init tentative parse
fallbacks that still special-case `identifier identifier` or grouped
pointer/reference starters before the new lexical scope state owns more of
that distinction directly.

## Watchouts
Keep lexical scope lookup separate from namespace traversal. The shared helper
is only safe for declaration/type-head probes plus the statement-side tail
position checks that branch to expression parsing after a value-classified
head. The constructor-init probe still special-cases unresolved
`identifier identifier`, `identifier &/* identifier`, simple parenthesized
`identifier(...)`, and grouped pointer/reference `identifier((...))`
starters differently, and qualified call-like statement probes still rely on
the later `classify_visible_value_or_type_starter()` branch when the token
after the qualified head is `(` or a value-like template call.

## Proof
`cmake --build --preset default` passed, and
`ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' | tee test_after.log`
passed. Added a focused frontend parser regression that keeps
`api::payload.value = 9;` on the expression-statement path while the shared
qualified visible-head classifier now supplies the statement parser with the
post-head tail position directly. Proof log: `test_after.log`.
