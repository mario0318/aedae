#include "../../src/Vault/KeyProtection.h"

#include <windows.h>
#include <bcrypt.h>
#include <wincrypt.h>
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

#pragma comment(lib, "Bcrypt.lib")

namespace
{
using namespace aedae;

void Check(bool condition, const char* name)
{
    if (!condition) throw std::runtime_error(name);
    std::cout << "PASS " << name << std::endl;
}

std::vector<std::uint8_t> Random(std::size_t size)
{
    std::vector<std::uint8_t> bytes(size);
    if (BCryptGenRandom(nullptr, bytes.data(), static_cast<ULONG>(bytes.size()),
        BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0)
        throw std::runtime_error("test random generation");
    return bytes;
}

bool Contains(std::span<const std::uint8_t> haystack, std::span<const std::uint8_t> needle)
{
    return std::search(haystack.begin(), haystack.end(), needle.begin(), needle.end()) != haystack.end();
}

std::uint32_t ReadU32(std::span<const std::uint8_t> bytes, std::size_t offset)
{
    if (offset > bytes.size() || bytes.size() - offset < 4)
        throw std::runtime_error("test envelope field");
    return static_cast<std::uint32_t>(bytes[offset]) |
        (static_cast<std::uint32_t>(bytes[offset + 1]) << 8) |
        (static_cast<std::uint32_t>(bytes[offset + 2]) << 16) |
        (static_cast<std::uint32_t>(bytes[offset + 3]) << 24);
}

void RoundTrips()
{
    DpapiKeyProtection protection;
    for (const std::size_t size : {std::size_t{1}, std::size_t{32}, std::size_t{4096}})
    {
        auto plaintext = Random(size);
        auto first = protection.ProtectPrivateKey(plaintext);
        auto second = protection.ProtectPrivateKey(plaintext);
        Check(static_cast<bool>(first) && static_cast<bool>(second), "random buffer protected");
        Check(*first.value != plaintext && (plaintext.size() < 32 || !Contains(*first.value, plaintext)),
            "plaintext absent from protected blob");
        Check(*first.value != *second.value, "repeated protection produces distinct envelopes");
        auto clear = protection.UnprotectPrivateKey(*first.value,
            KeyProtectionPurpose::credential_private_key);
        Check(static_cast<bool>(clear) && std::ranges::equal(clear.value->bytes(), plaintext),
            "random buffer round trip");
        SecureZeroMemory(plaintext.data(), plaintext.size());
    }
}

void InvalidInputs()
{
    DpapiKeyProtection protection;
    Check(protection.ProtectPrivateKey({}).error == KeyProtectionError::invalid_argument,
        "empty plaintext refused");
    auto tooLarge = Random(1024 * 1024 + 1);
    Check(protection.ProtectPrivateKey(tooLarge).error == KeyProtectionError::resource_limit,
        "oversized plaintext refused");
    SecureZeroMemory(tooLarge.data(), tooLarge.size());
    Check(protection.UnprotectPrivateKey({}, KeyProtectionPurpose::credential_private_key).error ==
        KeyProtectionError::malformed_blob, "empty protected blob refused");
}

void WrongPurposeAndTamper()
{
    DpapiKeyProtection protection;
    auto plaintext = Random(96);
    auto protectedResult = protection.ProtectPrivateKey(plaintext);
    Check(static_cast<bool>(protectedResult), "tamper fixture protected");
    const auto original = *protectedResult.value;
    Check(protection.UnprotectPrivateKey(original, static_cast<KeyProtectionPurpose>(99)).error ==
        KeyProtectionError::authentication_failed, "wrong protection purpose fails closed");

    const auto wrappedBytes = ReadU32(original, 16);
    auto wrongEntropyBytes = Random(46);
    DATA_BLOB wrapped{wrappedBytes, const_cast<BYTE*>(original.data() + 32)};
    DATA_BLOB wrongEntropy{static_cast<DWORD>(wrongEntropyBytes.size()), wrongEntropyBytes.data()};
    DATA_BLOB unexpectedlyClear{};
    const bool wrongEntropyAccepted = CryptUnprotectData(&wrapped, nullptr, &wrongEntropy, nullptr,
        nullptr, CRYPTPROTECT_UI_FORBIDDEN, &unexpectedlyClear) != FALSE;
    if (unexpectedlyClear.pbData)
    {
        SecureZeroMemory(unexpectedlyClear.pbData, unexpectedlyClear.cbData);
        LocalFree(unexpectedlyClear.pbData);
    }
    SecureZeroMemory(wrongEntropyBytes.data(), wrongEntropyBytes.size());
    Check(!wrongEntropyAccepted, "DPAPI wrapped key rejects wrong entropy context");

    bool everyTamperRefused = true;
    for (std::size_t index = 0; index < original.size(); ++index)
    {
        auto changed = original;
        changed[index] ^= 0x80;
        everyTamperRefused = everyTamperRefused && !protection.UnprotectPrivateKey(changed,
            KeyProtectionPurpose::credential_private_key);
    }
    Check(everyTamperRefused, "one deterministic bit flip at every byte position refused");
    bool everyTruncationRefused = true;
    for (std::size_t size = 0; size < original.size(); ++size)
        everyTruncationRefused = everyTruncationRefused &&
            !protection.UnprotectPrivateKey(std::span(original).first(size),
                KeyProtectionPurpose::credential_private_key);
    Check(everyTruncationRefused, "every truncation refused");
    auto trailing = original;
    trailing.push_back(0);
    Check(!protection.UnprotectPrivateKey(trailing,
        KeyProtectionPurpose::credential_private_key), "trailing data refused");
    SecureZeroMemory(plaintext.data(), plaintext.size());
}
}

int main()
{
    try
    {
        RoundTrips();
        InvalidInputs();
        WrongPurposeAndTamper();
        std::cout << "KeyProtectionTests passed; random synthetic buffers only" << std::endl;
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL " << error.what() << std::endl;
        return 1;
    }
}
