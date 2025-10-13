#!/usr/bin/env python3
"""
Simple static audit for target-specific API usage.

Scans C/C++ source files and reports matches for a set of regexes that commonly
indicate chip-specific assumptions (for example accessing non-portable struct
fields like `.bandwidth`).

Usage: python3 tools/audit_target_api.py
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Patterns to search for. Keep them specific to reduce false positives.
# Each entry: (pattern, message, severity) where severity is 'error' or 'warning'
PATTERNS = [
    (re.compile(r"\bwifi_config\.ap\.bandwidth\b|\.ap\.bandwidth\b"),
     "Direct access to AP bandwidth field (e.g. wifi_config.ap.bandwidth). Use esp_wifi_set_bandwidth() instead",
     'error'),
    (re.compile(r"\besp_wifi_set_bandwidths?\s*\("),
     "Using esp_wifi_set_bandwidth* calls — ensure the chosen API is supported on all targets",
     'warning'),
    (re.compile(r"\bCONFIG_IDF_TARGET_\w+\b"),
     "Found target-specific CONFIG macros - ensure guarded usage and provide fallback",
     'warning'),
    # includes are handled specially (see allowlist below)
]

# Headers that are public and expected across targets; don't warn for these
ALLOWLIST_HEADERS = {
    'esp_log.h', 'esp_mac.h', 'esp_wifi.h', 'esp_event.h', 'esp_err.h', 'esp_system.h'
}

exts = {'.c', '.h', '.cpp', '.hpp', '.cc'}

def scan_file(path):
    try:
        with open(path, 'r', encoding='utf-8', errors='ignore') as f:
            lines = f.readlines()
    except Exception as e:
        return []
    hits = []
    for i, line in enumerate(lines, start=1):
        # check patterns
        for pat, msg, severity in PATTERNS:
            if pat.search(line):
                hits.append((i, line.rstrip('\n'), msg, pat.pattern, severity))
        # special-case esp_ includes: warn only if not allowlisted
        m = re.search(r"#include\s+\"(esp_[^\"]+\.h)\"", line)
        if m:
            hdr = m.group(1)
            if hdr not in ALLOWLIST_HEADERS:
                hits.append((i, line.rstrip('\n'), f"Including esp header {hdr} - verify it's public on all targets", hdr, 'warning'))
    return hits

def walk_and_scan(root):
    results = {}
    for dirpath, dirnames, filenames in os.walk(root):
        # skip build and .git
        if '/build/' in dirpath or '/.git/' in dirpath:
            continue
        for fn in filenames:
            if os.path.splitext(fn)[1] in exts:
                p = os.path.join(dirpath, fn)
                hits = scan_file(p)
                if hits:
                    results[p] = hits
    return results

def main():
    print("Scanning workspace for potential target-specific API usage...")
    results = walk_and_scan(ROOT)
    if not results:
        print("No suspicious patterns found.")
        return 0

    # Collect counts by severity
    total = 0
    counts = {'error': 0, 'warning': 0}
    print()
    for path, hits in sorted(results.items()):
        print(path)
        for hit in hits:
            lineno, text, msg, pat, severity = hit
            total += 1
            if severity in counts:
                counts[severity] += 1
            sev_tag = severity.upper() if severity else 'INFO'
            print(f"  {lineno:4d}: {text}")
            print(f"        -> [{sev_tag}] {msg}  (pattern: {pat})")
        print()

    print(f"Found {total} matches in {len(results)} files.")
    print(f"  Errors: {counts['error']}, Warnings: {counts['warning']}")
    if counts['error'] > 0:
        print("One or more ERROR-level matches found; failing with exit code 2")
        return 2
    return 0

if __name__ == '__main__':
    sys.exit(main())
