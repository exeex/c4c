Status: Active
Source Idea Path: ideas/open/128_parser_ast_handoff_role_labeling.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Label `Node`

# Current Packet

## Just Finished

Step 3 added behavior-preserving boundary-role labels for `Node` fields in
`src/frontend/parser/ast.hpp`.

The comment groups now distinguish cross-stage semantic payload, name/text
identity bridges, compatibility/display spelling, parser recovery/debug
payload, semantic hints/flags, and legacy bridge fields without changing
declarations or behavior.

## Suggested Next

Handle Step 4: review the edited labels for suspicious string or rendered
spelling authority and capture any needed follow-up as a narrow `ideas/open/`
idea without changing compiler behavior.

## Watchouts

- Keep this runbook behavior-preserving.
- Do not replace string lookup paths or change parser/Sema/HIR/codegen behavior.
- Record suspicious cross-stage string authority as follow-up idea work instead
  of fixing it ad hoc.
- `template_origin_name`, operator spelling, scalar raw lexeme spelling, and
  constructor/template/member names are now explicitly labeled as compatibility,
  display, identity-bridge, or legacy payload; Step 4 should decide whether any
  of those need follow-up idea coverage.

## Proof

Ran `cmake --build --preset default > test_after.log 2>&1`; it completed
successfully with exit code 0. `test_after.log` is the proof log path.
