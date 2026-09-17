#include <windows.h>
#ifdef GetCurrentTime
#undef GetCurrentTime
#endif
#include <microsoft.ui.xaml.window.h>
#include <winrt/base.h>
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Xaml.Automation.h>
#include <winrt/Microsoft.UI.Xaml.Automation.Peers.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.Markup.h>
#include <winrt/Microsoft.UI.Xaml.Media.h>
#include <winrt/Microsoft.UI.Xaml.XamlTypeInfo.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.UI.Text.h>
#include <winrt/Windows.UI.Xaml.Interop.h>
#include "IdentityHealthModel.h"
#include "MockThisPcService.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Automation;
using namespace Microsoft::UI::Xaml::Automation::Peers;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::XamlTypeInfo;
using namespace Windows::UI::Text;
using namespace Windows::UI::Xaml::Interop;

namespace
{
SolidColorBrush ThemeBrush(std::wstring_view resourceName)
{
    return Application::Current().Resources().Lookup(box_value(hstring(resourceName))).as<SolidColorBrush>();
}

TextBlock Text(std::wstring_view value, double size, FontWeight weight, SolidColorBrush const& foreground)
{
    TextBlock text;
    text.Text(value);
    text.FontSize(size);
    text.FontWeight(weight);
    text.Foreground(foreground);
    text.TextWrapping(TextWrapping::Wrap);
    return text;
}

ContentControl StatusCard(std::wstring_view label, std::wstring_view value, std::wstring_view detail,
                          SolidColorBrush const& accent, SolidColorBrush const& surface,
                          SolidColorBrush const& primary, SolidColorBrush const& secondary)
{
    StackPanel content;
    content.Spacing(5);
    auto labelText = Text(label, 12, FontWeights::SemiBold(), accent);
    auto valueText = Text(value, 19, FontWeights::SemiBold(), primary);
    auto detailText = Text(detail, 13, FontWeights::Normal(), secondary);
    AutomationProperties::SetAccessibilityView(labelText, AccessibilityView::Raw);
    AutomationProperties::SetAccessibilityView(valueText, AccessibilityView::Raw);
    AutomationProperties::SetAccessibilityView(detailText, AccessibilityView::Raw);
    content.Children().Append(labelText);
    content.Children().Append(valueText);
    content.Children().Append(detailText);

    ContentControl card;
    card.Background(surface);
    card.BorderBrush(accent);
    card.BorderThickness(Thickness{4, 0, 0, 0});
    card.CornerRadius(CornerRadius{0, 12, 12, 0});
    card.Padding(Thickness{20, 16, 20, 16});
    card.Content(content);
    card.IsTabStop(false);
    AutomationProperties::SetAccessibilityView(card, AccessibilityView::Content);
    AutomationProperties::SetName(card, hstring(label) + L": " + hstring(value) + L". " + hstring(detail));
    return card;
}

std::wstring JoinSources(std::vector<std::wstring> const& sources)
{
    std::wstring joined;
    for (std::size_t index = 0; index < sources.size(); ++index)
    {
        if (index != 0) joined += L", ";
        joined += sources[index];
    }
    return joined;
}

std::wstring WarningLabel(aedae::management::IdentityHealthWarningKind kind)
{
    using aedae::management::IdentityHealthWarningKind;
    switch (kind)
    {
    case IdentityHealthWarningKind::localOnly: return L"LOCAL-ONLY CREDENTIAL";
    case IdentityHealthWarningKind::singleKnownAuthenticator: return L"ONE KNOWN AUTHENTICATOR";
    case IdentityHealthWarningKind::duplicateCandidate: return L"POSSIBLE DUPLICATE";
    }
    return L"UNKNOWN REVIEW SIGNAL";
}

ContentControl WarningCard(aedae::management::IdentityHealthWarning const& warning,
                           SolidColorBrush const& accent, SolidColorBrush const& surface,
                           SolidColorBrush const& primary, SolidColorBrush const& secondary)
{
    const auto label = WarningLabel(warning.kind);
    const auto sources = JoinSources(warning.sourceCredentialRefs);
    StackPanel content;
    content.Spacing(5);
    auto labelText = Text(label, 12, FontWeights::SemiBold(), accent);
    auto ruleText = Text(L"Rule " + warning.ruleId, 16, FontWeights::SemiBold(), primary);
    auto sourceText = Text(L"Source records: " + sources, 13, FontWeights::Normal(), secondary);
    AutomationProperties::SetAccessibilityView(labelText, AccessibilityView::Raw);
    AutomationProperties::SetAccessibilityView(ruleText, AccessibilityView::Raw);
    AutomationProperties::SetAccessibilityView(sourceText, AccessibilityView::Raw);
    content.Children().Append(labelText);
    content.Children().Append(ruleText);
    content.Children().Append(sourceText);

    ContentControl card;
    card.Background(surface);
    card.BorderBrush(accent);
    card.BorderThickness(Thickness{4, 0, 0, 0});
    card.CornerRadius(CornerRadius{0, 12, 12, 0});
    card.Padding(Thickness{20, 14, 20, 14});
    card.Content(content);
    card.IsTabStop(false);
    AutomationProperties::SetAccessibilityView(card, AccessibilityView::Content);
    AutomationProperties::SetName(card, hstring(label) + L". Rule " + hstring(warning.ruleId) +
        L". Source records: " + hstring(sources));
    return card;
}

aedae::management::IdentityHealthResult SyntheticIdentityHealth()
{
    aedae::management::IdentityHealthInput input;
    input.credentials = {
        {L"synthetic-cred-1", L"example.test", L"synthetic-account-a",
            {{L"aeDae", L"this-pc", true}}},
        {L"synthetic-cred-2", L"example.test", L"synthetic-account-a",
            {{L"external-provider", L"synthetic-device-2", false}}},
        {L"synthetic-cred-3", L"other.test", L"synthetic-account-b",
            {{L"aeDae", L"this-pc", true}}}
    };
    return aedae::management::ComputeIdentityHealth(input);
}

StackPanel IdentityHealthSection(aedae::management::IdentityHealthResult const& result,
                                 SolidColorBrush const& accent, SolidColorBrush const& banner,
                                 SolidColorBrush const& surface, SolidColorBrush const& primary,
                                 SolidColorBrush const& secondary)
{
    using aedae::management::IdentityHealthStatus;
    StackPanel section;
    section.Spacing(12);
    section.Margin(Thickness{0, 22, 0, 0});
    section.Children().Append(Text(L"IDENTITY HEALTH  /  SYNTHETIC PREVIEW", 12,
                                   FontWeights::SemiBold(), accent));
    section.Children().Append(Text(L"Review signals, with their evidence attached.", 28,
                                   FontWeights::SemiBold(), primary));
    section.Children().Append(Text(L"These counts describe only the example records below. They are not a scan of this PC or another provider.",
                                   15, FontWeights::Normal(), secondary));

    if (!aedae::management::CanPresentIdentityHealthSummary(result))
    {
        ContentControl refused;
        refused.Background(ThemeBrush(L"SystemFillColorAttentionBackgroundBrush"));
        refused.CornerRadius(CornerRadius{10});
        refused.Padding(Thickness{18});
        refused.Content(Text(L"Identity Health refused: the input was live, invalid, or ambiguous.",
                             15, FontWeights::SemiBold(), primary));
        refused.IsTabStop(false);
        AutomationProperties::SetAccessibilityView(refused, AccessibilityView::Content);
        AutomationProperties::SetName(refused, L"Identity Health refused because the input was live, invalid, or ambiguous.");
        section.Children().Append(refused);
        return section;
    }

    const auto& summary = *result.summary;
    section.Children().Append(StatusCard(L"CREDENTIAL RECORDS", std::to_wstring(summary.totalCredentials),
        L"Synthetic records supplied to the model.", accent, surface, primary, secondary));
    section.Children().Append(StatusCard(L"KNOWN PROVIDERS", std::to_wstring(summary.knownProviders),
        L"Distinct provider IDs present in the synthetic source metadata.", accent, surface, primary, secondary));
    section.Children().Append(StatusCard(L"KNOWN DEVICES", std::to_wstring(summary.knownDevices),
        L"Distinct device IDs present in the synthetic source metadata.", accent, surface, primary, secondary));

    ContentControl limit;
    limit.Background(banner);
    limit.CornerRadius(CornerRadius{10});
    limit.Padding(Thickness{18, 12, 18, 12});
    limit.Content(Text(summary.visibilityLimit, 13, FontWeights::SemiBold(), primary));
    limit.IsTabStop(false);
    AutomationProperties::SetAccessibilityView(limit, AccessibilityView::Content);
    AutomationProperties::SetName(limit, summary.visibilityLimit);
    section.Children().Append(limit);

    section.Children().Append(Text(L"REVIEW SIGNALS — SIMULATED", 12, FontWeights::SemiBold(), accent));
    for (const auto& warning : summary.warnings)
        section.Children().Append(WarningCard(warning, accent, surface, primary, secondary));
    return section;
}

FrameworkElement BuildPage(aedae::management::ThisPcSnapshot const& snapshot,
                           aedae::management::IdentityHealthResult const& identityHealth)
{
    const auto pageBackground = ThemeBrush(L"ApplicationPageBackgroundThemeBrush");
    const auto surface = ThemeBrush(L"CardBackgroundFillColorDefaultBrush");
    const auto primary = ThemeBrush(L"TextFillColorPrimaryBrush");
    const auto secondary = ThemeBrush(L"TextFillColorSecondaryBrush");
    const auto accent = ThemeBrush(L"AccentTextFillColorPrimaryBrush");
    const auto banner = ThemeBrush(L"SystemFillColorNeutralBackgroundBrush");
    const auto locked = ThemeBrush(L"LayerFillColorDefaultBrush");

    StackPanel page;
    page.Spacing(16);
    page.MaxWidth(920);
    page.Margin(Thickness{44, 36, 44, 56});

    auto eyebrow = Text(L"aeDae  /  THIS PC", 12, FontWeights::SemiBold(), accent);
    eyebrow.CharacterSpacing(115);
    page.Children().Append(eyebrow);

    page.Children().Append(Text(L"Your authenticator, without invented confidence.", 34,
                                FontWeights::SemiBold(), primary));
    page.Children().Append(Text(L"This preview shows exactly what the application knows today—and leaves everything else unknown.",
                                16, FontWeights::Normal(), secondary));

    const auto presentation = aedae::management::DecideThisPcPresentation(snapshot);
    if (presentation.showRefusal)
    {
        ContentControl refused;
        refused.Background(ThemeBrush(L"SystemFillColorAttentionBackgroundBrush"));
        refused.CornerRadius(CornerRadius{10});
        refused.Padding(Thickness{18});
        refused.Content(Text(L"Preview refused. Live status is outside this build's approved scope.",
                             16, FontWeights::SemiBold(), primary));
        refused.IsTabStop(false);
        AutomationProperties::SetAccessibilityView(refused, AccessibilityView::Content);
        AutomationProperties::SetName(refused, L"Preview refused because live status is outside the approved scope.");
        page.Children().Append(refused);
    }
    else
    {
        ContentControl provenance;
        provenance.Background(banner);
        provenance.CornerRadius(CornerRadius{10});
        provenance.Padding(Thickness{18, 12, 18, 12});
        provenance.Content(Text(L"MOCK DATA ONLY  ·  not a live security assessment", 13,
                                FontWeights::SemiBold(), primary));
        provenance.IsTabStop(false);
        AutomationProperties::SetAccessibilityView(provenance, AccessibilityView::Content);
        AutomationProperties::SetName(provenance, L"Mock data only. Not a live security assessment.");
        page.Children().Append(provenance);

        const std::wstring provider = snapshot.registered ? L"Registered (simulated)" : L"Not registered (simulated)";
        const std::wstring hello = aedae::management::AvailabilityLabel(snapshot.hello);
        const std::wstring protection = aedae::management::AvailabilityLabel(snapshot.keyProtection);
        const std::wstring count = snapshot.localCredentialCount
            ? std::to_wstring(*snapshot.localCredentialCount) + L" (synthetic)"
            : L"Unknown (not queried)";

        page.Children().Append(StatusCard(L"PROVIDER", provider,
            L"No registration or Windows settings query was performed.", accent, surface, primary, secondary));
        page.Children().Append(StatusCard(L"WINDOWS HELLO", hello,
            L"Biometric and PIN capability have not been queried.", accent, surface, primary, secondary));
        page.Children().Append(StatusCard(L"KEY PROTECTION", protection,
            L"No key, TPM, DPAPI, or vault operation was attempted.", accent, surface, primary, secondary));
        page.Children().Append(StatusCard(L"LOCAL CREDENTIALS", count,
            L"Unknown is intentionally different from a known zero.", accent, surface, primary, secondary));
    }

    Border actions;
    actions.Background(locked);
    actions.CornerRadius(CornerRadius{12});
    actions.Padding(Thickness{20});
    StackPanel actionContent;
    actionContent.Spacing(10);
    actionContent.Children().Append(Text(L"Credential operations stay locked", 18,
                                         FontWeights::SemiBold(), primary));
    actionContent.Children().Append(Text(L"Enable, disable, delete, registration, signing, and key access remain unavailable until their separate security gates pass.",
                                         14, FontWeights::Normal(), secondary));
    Button manage;
    manage.Content(box_value(L"Manage credentials"));
    manage.IsEnabled(false);
    manage.HorizontalAlignment(HorizontalAlignment::Left);
    AutomationProperties::SetName(manage, L"Manage credentials. Disabled pending security review.");
    actionContent.Children().Append(manage);
    actions.Child(actionContent);
    page.Children().Append(actions);
    page.Children().Append(IdentityHealthSection(identityHealth, accent, banner, surface, primary, secondary));

    ScrollViewer scroller;
    scroller.Background(pageBackground);
    scroller.VerticalScrollBarVisibility(ScrollBarVisibility::Auto);
    scroller.HorizontalScrollBarVisibility(ScrollBarVisibility::Disabled);
    scroller.Content(page);
    AutomationProperties::SetName(scroller, L"This PC authenticator status preview");
    return scroller;
}

class ManagementApplication : public ApplicationT<ManagementApplication, IXamlMetadataProvider>
{
public:
    void OnLaunched(LaunchActivatedEventArgs const&)
    {
        Resources().MergedDictionaries().Append(XamlControlsResources());
        window_ = Window();
        window_.Title(L"aeDae — This PC");
        RenderPage();
        window_.Activate();

        HWND hwnd{};
        check_hresult(window_.as<::IWindowNative>()->get_WindowHandle(&hwnd));
        const auto dpi = GetDpiForWindow(hwnd);
        const auto width = MulDiv(1040, static_cast<int>(dpi), 96);
        const auto height = MulDiv(780, static_cast<int>(dpi), 96);
        check_bool(SetWindowPos(hwnd, nullptr, 0, 0, width, height,
                                SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE));
    }

    IXamlType GetXamlType(TypeName const& type) { return provider_.GetXamlType(type); }
    IXamlType GetXamlType(hstring const& name) { return provider_.GetXamlType(name); }
    com_array<XmlnsDefinition> GetXmlnsDefinitions() { return provider_.GetXmlnsDefinitions(); }

private:
    void RenderPage()
    {
        const aedae::management::MockThisPcStatusService service;
        auto page = BuildPage(service.GetSnapshot(), SyntheticIdentityHealth());
        page.ActualThemeChanged({this, &ManagementApplication::OnActualThemeChanged});
        window_.Content(page);
    }

    void OnActualThemeChanged(FrameworkElement const& source, IInspectable const&)
    {
        if (source.ActualTheme() == renderedTheme_) return;
        renderedTheme_ = source.ActualTheme();
        RenderPage();
    }

    Window window_{nullptr};
    ElementTheme renderedTheme_{ElementTheme::Default};
    XamlControlsXamlMetaDataProvider provider_;
};
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    init_apartment(apartment_type::single_threaded);
    Application::Start([](auto&&) { make<ManagementApplication>(); });
    return 0;
}
