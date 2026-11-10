import json
import os
import subprocess
import sys
import tempfile
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import plan_matrix  # noqa: E402


class PlanMatrix(unittest.TestCase):
    def test_blank_builds_every_entry(self):
        names = [plan_matrix.name_of(e) for e in plan_matrix.plan("")]
        self.assertEqual(names, ["linux", "macos", "windows", "windows-flowquic"])

    def test_whitespace_is_blank(self):
        self.assertEqual(len(plan_matrix.plan("  ")), 4)

    def test_only_names_one_entry(self):
        chosen = plan_matrix.plan("windows-flowquic")
        self.assertEqual(len(chosen), 1)
        self.assertEqual(chosen[0]["flowquic"], "ON")

    def test_only_os_name_does_not_match_the_labelled_entry(self):
        chosen = plan_matrix.plan("windows")
        self.assertEqual(len(chosen), 1)
        self.assertNotIn("flowquic", chosen[0])

    def test_unknown_name_is_an_error_not_an_empty_matrix(self):
        with self.assertRaises(SystemExit) as raised:
            plan_matrix.plan("nope")
        self.assertIn("no matrix entry is named nope", str(raised.exception))

    def test_every_entry_has_what_the_workflow_reads(self):
        for entry in plan_matrix.ENTRIES:
            for key in ("os", "runner", "container", "build_jobs"):
                self.assertIn(key, entry, plan_matrix.name_of(entry))

    def test_only_the_flowquic_entry_turns_flowquic_on(self):
        on = [e for e in plan_matrix.ENTRIES if e.get("flowquic") == "ON"]
        self.assertEqual([plan_matrix.name_of(e) for e in on], ["windows-flowquic"])

    def test_the_script_writes_json_the_workflow_can_read(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = os.path.join(tmp, "output")
            env = dict(os.environ, ONLY="windows-flowquic", GITHUB_OUTPUT=out)
            subprocess.run(
                [sys.executable, os.path.join(HERE, "plan_matrix.py")],
                env=env,
                check=True,
                capture_output=True,
            )
            line = open(out).read().strip()
        self.assertTrue(line.startswith("include="))
        parsed = json.loads(line[len("include=") :])
        self.assertEqual([e["label"] for e in parsed], ["windows-flowquic"])

    def test_the_script_fails_on_an_unknown_name(self):
        with tempfile.TemporaryDirectory() as tmp:
            env = dict(os.environ, ONLY="nope", GITHUB_OUTPUT=os.path.join(tmp, "o"))
            done = subprocess.run(
                [sys.executable, os.path.join(HERE, "plan_matrix.py")],
                env=env,
                capture_output=True,
            )
        self.assertNotEqual(done.returncode, 0)


if __name__ == "__main__":
    unittest.main()
