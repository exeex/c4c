# c4c Use Cases

This document gives simple entry points for common user intents.

## I Just Started. What Can I Do In This Project?

Ask this when you want orientation before making changes.

The answer should usually cover:

- what the main subsystems are
- what active work is in progress
- which source files or docs are best to read first
- whether there is an existing plan already in motion

Recommended follow-up prompts:

- `Show me the current plan state and explain it in plain language.`
- `Give me a guided reading path for the most important source files.`
- `What is a good first small task for learning this codebase?`

## I Want To Implement XXX. How Should I Do It?

Ask this when you already have a feature goal.

The answer should usually cover:

- where the feature belongs in the codebase
- whether it fits the active plan or should become a new idea
- what tests should be added or updated
- which risks or design choices matter most

Recommended follow-up prompts:

- `Find the files most relevant to implementing <feature>.`
- `Sketch a minimal implementation plan for <feature>.`
- `Implement the first safe step and explain the change.`

## I Want To Connect c4c To My Custom Hardware. How Should I Approach It?

Ask this when you are doing backend, target, or hardware integration work.

The answer should usually cover:

- what target-specific pieces need to exist
- whether parser/sema/IR/codegen changes are needed
- how to debug the compiler pipeline during bring-up
- how to validate progress with focused tests

Recommended follow-up prompts:

- `Which compiler stages are affected by supporting my hardware target?`
- `Help me make a bring-up checklist for this custom target.`
- `Show me the debug flags I should use while validating parse/sema/HIR/codegen output.`

## Suggested Future Expansion

This file is intentionally small. Good future sections would be:

- adding a new language feature
- fixing a miscompile
- improving diagnostics
- adding a new backend or target
- validating regressions before landing changes
