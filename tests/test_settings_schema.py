"""The settings schema (mod.toml), NativeOptions.ini, its alias table and the settings guide
must not drift. tools/check_settings.py is the checker."""
from pathlib import Path
import subprocess, sys

def test_settings_schema_has_no_drift():
    root = Path(__file__).resolve().parents[1]
    r = subprocess.run([sys.executable, str(root / 'tools/check_settings.py'), str(root)], capture_output=True, text=True)
    assert r.returncode == 0, r.stdout + r.stderr
    assert 'RESULT PASS' in r.stdout
    assert 'undocumented in both INI and guide: 0:' in r.stdout, r.stdout
