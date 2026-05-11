#!/usr/bin/env python3
"""Focused self-tests for string_authority_guard.py."""

from __future__ import annotations

import contextlib
import io
import json
import tempfile
import unittest
from pathlib import Path

import string_authority_guard


REQUIRED_CLASSIFICATION = {
    "pattern": "by-name-member",
    "owner": "self-test",
    "domain": "semantic declaration lookup fixture",
    "category": "compatibility-bridge",
    "reason": "Fixture entry proves exact path and symbol classifications pass.",
    "removal_condition": "Remove when fixture no longer needs this accepted hit.",
    "evidence": "The self-test fixture declares semantic_decls_by_name.",
}


class StringAuthorityGuardTest(unittest.TestCase):
    def write_fixture(self, root: Path) -> None:
        fixture = root / "src/frontend/sema/fixture.hpp"
        fixture.parent.mkdir(parents=True)
        fixture.write_text(
            """#pragma once
#include <string>
#include <unordered_map>

namespace c4c::fixture {
struct SemanticOwner {
  std::unordered_map<std::string, int> semantic_decls_by_name;
  std::unordered_map<std::string, int> unexpected_cache;
  std::unordered_map<std::string, int> display_name_map;
  std::unordered_map<std::string, int> route_local_by_name;
  std::unordered_map<std::string, int> diagnostic_name_map;
};
}
""",
            encoding="utf-8",
        )

    def write_config(self, root: Path, entries: list[dict[str, str]]) -> Path:
        config = root / "scripts/string_authority_classifications.json"
        config.parent.mkdir()
        config.write_text(
            json.dumps(
                {
                    "version": 1,
                    "roots": ["src/frontend/sema"],
                    "classifications": entries,
                },
                indent=2,
            ),
            encoding="utf-8",
        )
        return config

    def run_guard(self, root: Path, config: Path) -> tuple[int, str]:
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            status = string_authority_guard.run_guard(root, config)
        return status, output.getvalue()

    def test_unclassified_semantic_string_map_fails(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self.write_fixture(root)
            config = self.write_config(root, [])

            status, output = self.run_guard(root, config)

            self.assertEqual(status, 1)
            self.assertIn("src/frontend/sema/fixture.hpp:7", output)
            self.assertIn("SemanticOwner::semantic_decls_by_name", output)
            self.assertIn("classify this exact path+symbol", output)

    def test_unclassified_generic_string_map_fails(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self.write_fixture(root)
            config = self.write_config(root, [])

            status, output = self.run_guard(root, config)

            self.assertEqual(status, 1)
            self.assertIn("src/frontend/sema/fixture.hpp:8", output)
            self.assertIn("SemanticOwner::unexpected_cache", output)
            self.assertIn("pattern=string-keyed-container", output)

    def test_classified_semantic_string_map_passes(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self.write_fixture(root)
            entries = [
                {
                    "path": "src/frontend/sema/fixture.hpp",
                    "symbol": "SemanticOwner::semantic_decls_by_name",
                    **REQUIRED_CLASSIFICATION,
                },
                {
                    "path": "src/frontend/sema/fixture.hpp",
                    "symbol": "SemanticOwner::unexpected_cache",
                    **REQUIRED_CLASSIFICATION,
                    "pattern": "string-keyed-container",
                    "category": "compatibility-bridge",
                },
                {
                    "path": "src/frontend/sema/fixture.hpp",
                    "symbol": "SemanticOwner::display_name_map",
                    **REQUIRED_CLASSIFICATION,
                    "category": "display-output",
                },
                {
                    "path": "src/frontend/sema/fixture.hpp",
                    "symbol": "SemanticOwner::route_local_by_name",
                    **REQUIRED_CLASSIFICATION,
                    "category": "route-local-identity",
                },
                {
                    "path": "src/frontend/sema/fixture.hpp",
                    "symbol": "SemanticOwner::diagnostic_name_map",
                    **REQUIRED_CLASSIFICATION,
                    "category": "diagnostic-debug",
                },
            ]
            config = self.write_config(root, entries)

            status, output = self.run_guard(root, config)

            self.assertEqual(status, 0)
            self.assertIn("passed: 5 classified", output)


if __name__ == "__main__":
    unittest.main()
