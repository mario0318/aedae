// Build-time checks only. This file intentionally implements no WebAuthn operation.
#include <cstddef>
#include <type_traits>
#include <unknwn.h>
#include <webauthn.h>
#include <webauthnplugin.h>
#include <pluginauthenticator.h>

static_assert(std::is_base_of_v<IUnknown, IPluginAuthenticator>);
static_assert(sizeof(WEBAUTHN_PLUGIN_ADD_AUTHENTICATOR_OPTIONS) == 72);
static_assert(offsetof(WEBAUTHN_PLUGIN_ADD_AUTHENTICATOR_OPTIONS, cbAuthenticatorInfo) == 40);
static_assert(offsetof(WEBAUTHN_PLUGIN_ADD_AUTHENTICATOR_OPTIONS, ppwszSupportedRpIds) == 64);
static_assert(sizeof(WEBAUTHN_PLUGIN_ADD_AUTHENTICATOR_RESPONSE) == 16);
static_assert(offsetof(WEBAUTHN_PLUGIN_ADD_AUTHENTICATOR_RESPONSE, pbOpSignPubKey) == 8);
