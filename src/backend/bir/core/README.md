# Core BIR Contract

Status: scaffold over an existing partial implementation.

Core owns module symbols, resolved types, globals, functions, blocks,
instructions, values, locals, terminators, stable IDs, order lists, builders,
editors, def-use, and function/module revisions.

Review must reconcile the existing headers in this directory with the 715
contract: generation-checked identity, mutation-scoped references,
transactional editing, complete operand/successor traversal, RAUW, block split,
edge redirect, and no direct persistent pointer/index authority.

Core may not include compatibility, preparation, MIR, target, prealloc, or LIR
definitions. Debug names are attachments, not identity.
