# Microsoft Passkey Manager sample mapping

Bootstrap status: mapped by role only. No sample source has been copied into product code.

| Sample area | aeDae milestone | Decision |
| --- | --- | --- |
| COM activation | T-003 | Bootstrap-only activation exists; implement the official interface only after the security gates close |
| Plugin registration | T-004 | Feature probe now; call only after ABI review |
| Credential creation/assertion | T-008/T-009 | Excluded from bootstrap |
| Credential metadata | Future T-005 onward | Excluded from bootstrap |
| UI | T-010 | Console status shell only |

The Microsoft sample requires Windows SDK 10.0.26100.7175 or later and a supported Windows 11 build. The locked local SDK `10.0.26100.0` exposes `webauthnplugin.h`; aeDae uses it only for pinned build-time contract checks and must not recreate its declarations.
