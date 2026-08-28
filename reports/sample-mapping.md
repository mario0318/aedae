# Microsoft Passkey Manager sample mapping

Bootstrap status: mapped by role only. No sample source has been copied into product code.

| Sample area | aeDae milestone | Decision |
| --- | --- | --- |
| COM activation | T-003 | Adapt after the official `IPluginAuthenticator` contract is available locally |
| Plugin registration | T-004 | Feature probe now; call only after ABI review |
| Credential creation/assertion | T-008/T-009 | Excluded from bootstrap |
| Credential metadata | Future T-005 onward | Excluded from bootstrap |
| UI | T-010 | Console status shell only |

The Microsoft sample requires Windows SDK 10.0.26100.7175 or later and a supported Windows 11 build. The currently installed headers do not expose `webauthnplugin.h`, so aeDae must not recreate those declarations.

