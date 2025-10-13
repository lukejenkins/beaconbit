# Target API Audit

This repository includes a small audit script `tools/audit_target_api.py` which
searches sources for patterns that commonly indicate target-specific API
assumptions. It is intended as a lightweight, static check to find places that
need conditional compilation or replacement with cross-target APIs.

How to run

```sh
python3 tools/audit_target_api.py
```

Common findings and remediation suggestions

- Direct struct field access (e.g. `.bandwidth`): use public APIs instead (for
  width use `esp_wifi_set_bandwidth(...)`).
- `CONFIG_IDF_TARGET_*` macros: ensure they are used to guard target-specific
  code paths and provide fallbacks.
- Header includes: prefer public headers (explicitly include `esp_mac.h` where
  needed).

Use the output of the script to guide fixes. After making changes, run the
script again to verify there are no remaining suspicious patterns.
