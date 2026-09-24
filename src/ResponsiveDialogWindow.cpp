#include "ResponsiveDialogWindow.hpp"

#include <algorithm>
#include <atomic>
#include <vector>

#if JUCE_WINDOWS
#include <shobjidl.h>
#endif
#if JUCE_ANDROID
#include <jni.h>
#include <juce_core/native/juce_JNIHelpers_android.h>
#endif

namespace
{
std::vector<juce::Component::SafePointer<ResponsiveDialogWindow>>& activeKeyboardDialogs()
{
    static std::vector<juce::Component::SafePointer<ResponsiveDialogWindow>> dialogs;
    return dialogs;
}

#if JUCE_WINDOWS
class WindowsInputPaneHandler final : public IFrameworkInputPaneHandler
{
public:
    explicit WindowsInputPaneHandler(ResponsiveDialogWindow& dialog) : target(&dialog) {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** object) override
    {
        if (object == nullptr)
            return E_POINTER;
        *object = nullptr;
        if (iid == IID_IUnknown || iid == IID_IFrameworkInputPaneHandler)
        {
            *object = static_cast<IFrameworkInputPaneHandler*>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override { return ++referenceCount; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        const auto remaining = --referenceCount;
        if (remaining == 0)
            delete this;
        return remaining;
    }

    HRESULT STDMETHODCALLTYPE Showing(RECT* bounds, BOOL) override
    {
        if (bounds != nullptr)
            send(*bounds);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE Hiding(BOOL) override
    {
        ResponsiveDialogWindow::notifyWindowsKeyboardOcclusion({}, target);
        return S_OK;
    }

private:
    void send(const RECT& bounds)
    {
        const juce::Rectangle<int> physical(bounds.left, bounds.top,
                                            bounds.right - bounds.left, bounds.bottom - bounds.top);
        ResponsiveDialogWindow::notifyWindowsKeyboardOcclusion(physical, target);
    }

    juce::Component::SafePointer<ResponsiveDialogWindow> target;
    std::atomic<ULONG> referenceCount{ 1 };
};
#endif

} // namespace

struct PlatformKeyboardSubscription
{
#if JUCE_WINDOWS
    explicit PlatformKeyboardSubscription(ResponsiveDialogWindow& dialog)
    {
        const auto initResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (FAILED(initResult))
        {
            juce::Logger::writeToLog("ResponsiveDialogWindow: COM STA initialization failed; Windows IME tracking is unavailable.");
            return;
        }
        shouldUninitialize = true;

        const auto createResult = CoCreateInstance(CLSID_FrameworkInputPane, nullptr, CLSCTX_INPROC_SERVER,
                                                   IID_PPV_ARGS(&inputPane));
        if (FAILED(createResult))
        {
            juce::Logger::writeToLog("ResponsiveDialogWindow: FrameworkInputPane creation failed; Windows IME tracking is unavailable.");
            return;
        }

        handler = new WindowsInputPaneHandler(dialog);
        if (auto* peer = dialog.getPeer())
        {
            const auto window = static_cast<HWND>(peer->getNativeHandle());
            if (window != nullptr && SUCCEEDED(inputPane->AdviseWithHWND(window, handler, &cookie)))
                advised = true;
        }
        if (advised)
        {
            // Advise reports future changes only. Query once so a dialog opened
            // while the touch keyboard is visible receives its current state.
            RECT bounds{};
            if (SUCCEEDED(inputPane->Location(&bounds))
                && bounds.right > bounds.left && bounds.bottom > bounds.top)
                handler->Showing(&bounds, FALSE);
        }
        if (!advised)
            juce::Logger::writeToLog("ResponsiveDialogWindow: FrameworkInputPane subscription failed; Windows IME tracking is unavailable.");
    }

    ~PlatformKeyboardSubscription()
    {
        if (inputPane != nullptr && advised)
            inputPane->Unadvise(cookie);
        if (handler != nullptr)
            handler->Release();
        if (inputPane != nullptr)
            inputPane->Release();
        if (shouldUninitialize)
            CoUninitialize();
    }

    IFrameworkInputPane* inputPane = nullptr;
    WindowsInputPaneHandler* handler = nullptr;
    DWORD cookie = 0;
    bool advised = false;
    bool shouldUninitialize = false;
#endif
};

namespace
{
int measureTextWidth(const juce::String& text, float height)
{
    juce::GlyphArrangement glyphs;
    glyphs.addLineOfText(juce::Font(juce::FontOptions{}.withHeight(height)), text, 0.0f, 0.0f);
    return juce::roundToInt(glyphs.getBoundingBox(0, glyphs.getNumGlyphs(), true).getWidth());
}

int measureWrappedTextHeight(const juce::String& text, int width, float fontHeight)
{
    juce::AttributedString attributed;
    attributed.append(text, juce::Font(juce::FontOptions{}.withHeight(fontHeight)), juce::Colours::white);
    juce::TextLayout layout;
    layout.createLayout(attributed, static_cast<float>(juce::jmax(1, width)));
    return juce::jmax(1, juce::roundToInt(layout.getHeight()));
}
}

void CenteredPopupLookAndFeel::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                                                  bool isSeparator, bool isActive, bool isHighlighted,
                                                  bool isTicked, bool hasSubMenu, const juce::String& text,
                                                  const juce::String&, const juce::Drawable*, const juce::Colour* textColour)
{
    if (isSeparator)
    {
        g.setColour(juce::Colours::grey);
        g.fillRect(area.reduced(8, area.getHeight() / 2));
        return;
    }
    g.setColour(isHighlighted ? findColour(juce::PopupMenu::highlightedBackgroundColourId)
                              : findColour(juce::PopupMenu::backgroundColourId));
    g.fillRect(area);
    g.setColour(textColour != nullptr ? *textColour : findColour(juce::PopupMenu::textColourId));
    g.setFont(juce::Font(juce::FontOptions{}.withHeight(14.0f)));
    if (isTicked)
    {
        juce::GlyphArrangement glyphs;
        glyphs.addLineOfText(g.getCurrentFont(), text, 0.0f, 0.0f);
        const auto textWidth = juce::roundToInt(glyphs.getBoundingBox(0,
                                                                       glyphs.getNumGlyphs(),
                                                                       true).getWidth());
        const auto checkArea = area.withX(area.getCentreX() - (textWidth / 2) - 28).withWidth(24);
        g.setFont(juce::Font(juce::FontOptions{}.withHeight(18.0f)));
        g.drawText(juce::CharPointer_UTF8("✓"), checkArea, juce::Justification::centred, false);
        g.setFont(juce::Font(juce::FontOptions{}.withHeight(14.0f)));
    }
    g.drawText(text, area.reduced(8, 0), juce::Justification::centred, true);
}

ResponsiveDialogWindow::ResponsiveDialogWindow(juce::String dialogTitle, juce::String dialogMessage) :
    title(std::move(dialogTitle)),
    message(std::move(dialogMessage))
{
    addAndMakeVisible(viewport);
    addAndMakeVisible(buttonBar);
    viewport.setScrollBarsShown(true, false);
    if (message.isNotEmpty())
    {
        messageLabel = std::make_unique<juce::TextEditor>();
        messageLabel->setText(message, juce::dontSendNotification);
        messageLabel->setFont(juce::Font(juce::FontOptions{}.withHeight(16.0f)));
        messageLabel->setMultiLine(true, true);
        messageLabel->setReadOnly(true);
        messageLabel->setScrollbarsShown(false);
        messageLabel->setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
        messageLabel->setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
        messageLabel->setColour(juce::TextEditor::textColourId, juce::Colours::white);
        content.addAndMakeVisible(*messageLabel);
    }
    viewport.setViewedComponent(&content, false);
    setOpaque(true);
}

ResponsiveDialogWindow::~ResponsiveDialogWindow()
{
    for (const auto& field : fields)
        if (auto* combo = dynamic_cast<juce::ComboBox*>(field.component))
            combo->setLookAndFeel(nullptr);
}

void ResponsiveDialogWindow::addTextBlock(const juce::String& text)
{
	    auto label = std::make_unique<juce::TextEditor>();
	    label->setText(text, false);
	    label->setName(text);
	    label->setFont(juce::Font(juce::FontOptions{}.withHeight(16.0f)));
    label->setMultiLine(true, true);
	    label->setReadOnly(true);
	    label->setScrollbarsShown(false);
	    label->setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
	    label->setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
	    label->setColour(juce::TextEditor::textColourId, juce::Colours::white);
	    fields.push_back({ {}, {}, label.get(), nullptr });
    content.addAndMakeVisible(*label);
    ownedComponents.push_back(std::move(label));
}

juce::TextEditor* ResponsiveDialogWindow::addTextEditor(const juce::String& key, const juce::String& value, const juce::String& label)
{
    auto editor = std::make_unique<juce::TextEditor>();
    editor->setText(value, false);
    editor->setFont(juce::Font(juce::FontOptions{}.withHeight(16.0f)));
    editor->setJustification(juce::Justification::centred);
    auto* result = editor.get();
    auto fieldLabel = std::make_unique<juce::Label>();
    fieldLabel->setText(label.isEmpty() ? key : label, juce::dontSendNotification);
    fieldLabel->setFont(juce::Font(juce::FontOptions{}.withHeight(16.0f)));
    fieldLabel->setJustificationType(juce::Justification::centred);
    auto* fieldLabelRaw = fieldLabel.get();
    content.addAndMakeVisible(*fieldLabel);
    fields.push_back({ key, label.isEmpty() ? key : label, result, fieldLabelRaw });
    ownedComponents.push_back(std::move(fieldLabel));
    content.addAndMakeVisible(*editor);
    ownedComponents.push_back(std::move(editor));
    return result;
}

juce::ComboBox* ResponsiveDialogWindow::addComboBox(const juce::String& key, const juce::StringArray& choices, const juce::String& label)
{
    auto combo = std::make_unique<juce::ComboBox>();
    combo->addItemList(choices, 1);
    combo->setJustificationType(juce::Justification::centred);
    combo->setLookAndFeel(&centeredPopupLookAndFeel);
    auto* result = combo.get();
    auto fieldLabel = std::make_unique<juce::Label>();
    fieldLabel->setText(label.isEmpty() ? key : label, juce::dontSendNotification);
    fieldLabel->setFont(juce::Font(juce::FontOptions{}.withHeight(16.0f)));
    fieldLabel->setJustificationType(juce::Justification::centred);
    auto* fieldLabelRaw = fieldLabel.get();
    content.addAndMakeVisible(*fieldLabel);
    fields.push_back({ key, label.isEmpty() ? key : label, result, fieldLabelRaw });
    ownedComponents.push_back(std::move(fieldLabel));
    content.addAndMakeVisible(*combo);
    ownedComponents.push_back(std::move(combo));
    return result;
}

juce::TextButton* ResponsiveDialogWindow::addButton(const juce::String& name, int result, std::function<bool()> canClose)
{
    auto button = std::make_unique<juce::TextButton>(name);
    auto* raw = button.get();
    raw->onClick = [this, result, canClose] { if (!canClose || canClose()) close(result); };
    buttonBar.addAndMakeVisible(*button);
    buttons.push_back({ raw, result, std::move(canClose) });
    ownedComponents.push_back(std::move(button));
    return raw;
}

void ResponsiveDialogWindow::addCustomComponent(juce::Component* component, int preferredHeight, int spacingAfter,
                                                int preferredWidth, bool fillRemainingViewportHeight,
                                                int minimumHeight)
{
    if (component == nullptr) return;
    content.addAndMakeVisible(*component);
    fields.push_back({ "custom", {}, component, nullptr, preferredHeight, spacingAfter, preferredWidth,
                       fillRemainingViewportHeight, juce::jmax(0, minimumHeight) });
}

void ResponsiveDialogWindow::updateCustomComponentHeight(juce::Component* component, int preferredHeight)
{
    for (auto& field : fields)
    {
        if (field.component != component || field.labelComponent != nullptr)
            continue;

        field.preferredHeight = juce::jmax(1, preferredHeight);
        if (!isVisible())
            return;

        const auto display = availableArea();
        const auto oldBounds = getBounds();
        resized();

        const int desiredHeight = contentHeight + titleAreaHeight()
            + (buttons.empty() ? buttonToDialogBottomGap
                               : contentToButtonGap + dialogButtonHeight + buttonToDialogBottomGap);
        const int fittedHeight = juce::jmin(desiredHeight, display.getHeight());
        setBounds(oldBounds.withSizeKeepingCentre(getWidth(), fittedHeight));
        resized();
        return;
    }
}

void ResponsiveDialogWindow::setInfoIconVisible(bool visible)
{
    infoIconVisible = visible;
    repaint();
}

void ResponsiveDialogWindow::setTitleVisible(bool visible)
{
    if (titleVisible == visible)
        return;

    titleVisible = visible;
    if (isVisible())
    {
        resized();
        repaint();
    }
}

void ResponsiveDialogWindow::setMessageVisibleAfterKeyboardDismiss(bool enabled)
{
    messageVisibleAfterKeyboardDismiss = enabled;
    if (!enabled || messageLabel == nullptr)
        return;

    messageVisible = false;
    messageLabel->setVisible(false);
    resized();
	const int desiredHeight = contentHeight + titleAreaHeight()
	    + (buttons.empty() ? buttonToDialogBottomGap
	                       : contentToButtonGap + dialogButtonHeight + buttonToDialogBottomGap);
	const auto area = availableArea();
	const auto* parent = getParentComponent();
	const int maxHeight = parent != nullptr ? juce::jmin(area.getHeight(), parent->getHeight()) : area.getHeight();
	const auto centre = parent != nullptr ? parent->getLocalBounds().getCentre() : getScreenBounds().getCentre();
	setSize(getWidth(), juce::jmin(desiredHeight, maxHeight));
	setCentrePosition(centre);
    repaint();
}
void ResponsiveDialogWindow::setVerticalScrollingEnabled(bool enabled)
{
    verticalScrollingEnabled = enabled;
    verticalScrollVisible = false;
    viewport.setScrollBarsShown(false, false);
    resized();
}

juce::String ResponsiveDialogWindow::getTextEditorContents(const juce::String& key) const
{
    if (auto* editor = getTextEditor(key)) return editor->getText();
    return {};
}

juce::TextEditor* ResponsiveDialogWindow::getTextEditor(const juce::String& key) const
{
    for (const auto& field : fields)
        if (field.key == key) return dynamic_cast<juce::TextEditor*>(field.component);
    return nullptr;
}

juce::ComboBox* ResponsiveDialogWindow::getComboBoxComponent(const juce::String& key) const
{
    for (const auto& field : fields)
        if (field.key == key) return dynamic_cast<juce::ComboBox*>(field.component);
    return nullptr;
}

void ResponsiveDialogWindow::showModal(juce::Component& parent, std::function<void(int)> resultCallback)
{
    callback = std::move(resultCallback);
    // Keep dialogs comfortably readable even when their content is short;
    // the measured content still determines the final size above this minimum.
    int desiredWidth = 320;
    desiredWidth = juce::jmax(desiredWidth, measureTextWidth(title, 16.0f) + 48);
    for (const auto& line : juce::StringArray::fromLines(message))
        desiredWidth = juce::jmax(desiredWidth, measureTextWidth(line, 16.0f) + 48);
    for (const auto& field : fields)
    {
        desiredWidth = juce::jmax(desiredWidth, measureTextWidth(field.label, 16.0f) + 48);
        if (field.preferredWidth > 0)
            desiredWidth = juce::jmax(desiredWidth, field.preferredWidth + 56);
        if (auto* editor = dynamic_cast<juce::TextEditor*>(field.component))
            desiredWidth = juce::jmax(desiredWidth, measureTextWidth(editor->getText(), 16.0f) + 48);
        if (auto* combo = dynamic_cast<juce::ComboBox*>(field.component))
            for (int i = 0; i < combo->getNumItems(); ++i)
                desiredWidth = juce::jmax(desiredWidth, measureTextWidth(combo->getItemText(i), 16.0f) + 96);
    }
    const auto display = availableArea();
    // Long descriptions should wrap instead of making the whole dialog wide.
    const int fittedWidth = juce::jmin(desiredWidth, juce::jmax(320, display.getWidth()), 480);

    // Give the content its final width first. Its layout then reports the
    // actual height, including labels and wrapped description text.
    setSize(fittedWidth, display.getHeight());
    resized();
    const int desiredHeight = contentHeight + titleAreaHeight()
        + (buttons.empty() ? buttonToDialogBottomGap
                           : contentToButtonGap + dialogButtonHeight + buttonToDialogBottomGap);
    const int fittedHeight = juce::jmin(desiredHeight, display.getHeight());
    setSize(fittedWidth, fittedHeight);
#if JUCE_ANDROID
    // Add the dialog only after its final size has been calculated. Showing a
    // full-height child first causes a visible resize flash on Android.
    parent.addAndMakeVisible(*this);
    setCentrePosition(parent.getLocalBounds().getCentre());
#else
    addToDesktop(juce::ComponentPeer::windowIsTemporary);
    setCentrePosition(parent.getScreenBounds().getCentre());
#endif
    setVisible(true);
    enterModalState(true);
    activeKeyboardDialogs().emplace_back(this);
#if JUCE_WINDOWS
    keyboardSubscription = std::make_unique<PlatformKeyboardSubscription>(*this);
#elif JUCE_ANDROID
    // Insets may not change when this dialog opens, so explicitly re-publish
    // the current IME state. This is a one-shot event request, not polling.
    auto* env = juce::getEnv();
    auto activity = juce::getCurrentActivity();
    if (activity == nullptr)
        activity = juce::getMainActivity();
    if (env != nullptr && activity != nullptr)
    {
        auto activityClass = env->GetObjectClass(activity.get());
        const auto method = activityClass != nullptr
            ? env->GetMethodID(activityClass, "requestCurrentImeInsets", "()V")
            : nullptr;
        if (method != nullptr)
            env->CallVoidMethod(activity.get(), method);
        if (env->ExceptionCheck())
            env->ExceptionClear();
        if (activityClass != nullptr)
            env->DeleteLocalRef(activityClass);
    }
#endif
}

void ResponsiveDialogWindow::close(int result)
{
    keyboardSubscription.reset();
    auto& activeDialogs = activeKeyboardDialogs();
    activeDialogs.erase(std::remove_if(activeDialogs.begin(), activeDialogs.end(),
                                      [this](const auto& dialog) { return dialog.getComponent() == this; }),
                        activeDialogs.end());
    if (keyboardWasVisible)
    {
        setBounds(boundsBeforeKeyboard);
        viewport.setViewPosition(scrollPositionBeforeKeyboard);
        keyboardWasVisible = false;
    }
    exitModalState(result);

    // The callback is allowed to destroy this dialog. Invoke it only after
    // all access to the dialog itself has completed.
    auto resultCallback = std::move(callback);
    if (resultCallback)
        resultCallback(result);
}

void ResponsiveDialogWindow::resized()
{
    const auto footer = buttons.empty() ? 0 : dialogFooterHeight;
    // The button row starts 10 px below its top. Keep 16 px between content
    // and the button, so the viewport bottom margin is 6 px.
    constexpr int viewportBottomMargin = 6;
    viewport.setBounds(dialogHorizontalMargin, titleAreaHeight(), getWidth() - 2 * dialogHorizontalMargin,
                       getHeight() - footer - titleAreaHeight() - dialogContentBottomMargin);
    buttonBar.setBounds(dialogHorizontalMargin, getHeight() - footer, getWidth() - 2 * dialogHorizontalMargin, footer);
    const int gap = 12;
    const int buttonWidth = juce::jmin(110, (buttonBar.getWidth() - gap * static_cast<int>(buttons.size() - 1)) /
                                             juce::jmax(1, static_cast<int>(buttons.size())));
    int buttonX = juce::jmax(0, (buttonBar.getWidth() - (buttonWidth * static_cast<int>(buttons.size()) + gap * static_cast<int>(buttons.size() - 1))) / 2);
    for (const auto& dialogButton : buttons)
    {
        auto* button = dialogButton.button;
        button->setBounds(buttonX, buttonToDialogBottomGap - 6, buttonWidth, dialogButtonHeight);
        buttonX += buttonWidth + gap;
    }
    layoutContent();
}

void ResponsiveDialogWindow::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff26343a));
    g.setColour(juce::Colours::lightgrey);
    g.drawRect(getLocalBounds().reduced(1), 1.0f);
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(juce::FontOptions{}.withHeight(16.0f)));
    if (titleVisible)
        g.drawFittedText(title, 16, 8, getWidth() - 32, 24, juce::Justification::centred, 1);

    if (infoIconVisible)
    {
        const auto iconBounds = juce::Rectangle<float>(18.0f, 5.0f, 34.0f, 34.0f);
        g.setColour(juce::Colour(0xff176e76));
        g.fillEllipse(iconBounds);
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(juce::FontOptions{}.withHeight(25.0f).withStyle("Bold")));
        g.drawText("i", iconBounds.toNearestInt(), juce::Justification::centred, false);
    }
}

void ResponsiveDialogWindow::layoutContent()
{
    int y = 8;
    if (messageLabel != nullptr && messageVisible)
    {
        const int messageWidth = juce::jmax(1, viewport.getWidth() - 24);
        const int messageHeight = measureWrappedTextHeight(message, messageWidth, 16.0f) + 8;
        messageLabel->setBounds(0, y, messageWidth, messageHeight);
        y += messageHeight + 10;
    }
    for (auto& field : fields)
    {
        if (field.component == nullptr) continue;
        if (field.labelComponent == nullptr && field.key.isEmpty())
        {
            const int textWidth = juce::jmax(1, viewport.getWidth() - 24);
            const int textHeight = measureWrappedTextHeight(field.component->getName(), textWidth, 16.0f) + 8;
            field.component->setBounds(0, y, textWidth, textHeight);
            y += textHeight + 12;
            continue;
        }
        if (field.labelComponent == nullptr)
        {
            const int availableWidth = juce::jmax(1, viewport.getWidth() - 24);
            const int componentWidth = field.preferredWidth > 0 ? juce::jmin(availableWidth, field.preferredWidth) : availableWidth;
            const int componentX = (availableWidth - componentWidth) / 2;
            const int componentHeight = field.fillRemainingViewportHeight
                ? juce::jmax(field.minimumHeight,
                             juce::jmin(field.preferredHeight,
                                        juce::jmax(1, viewport.getHeight() - y - field.spacingAfter - 8)))
                : field.preferredHeight;
            field.component->setBounds(componentX, y, componentWidth, componentHeight);
            y += componentHeight + field.spacingAfter;
            continue;
        }
        if (field.labelComponent != nullptr)
        {
            field.labelComponent->setBounds(0, y, juce::jmax(1, viewport.getWidth() - 24), 18);
            y += 20;
        }
        field.component->setBounds(0, y, juce::jmax(1, viewport.getWidth() - 24), 30);
        y += 42;
    }
    contentHeight = y + 8;
    const bool needsVerticalScroll = verticalScrollingEnabled && contentHeight > viewport.getHeight();
    if (needsVerticalScroll != verticalScrollVisible)
    {
        verticalScrollVisible = needsVerticalScroll;
        viewport.setScrollBarsShown(verticalScrollVisible, false);
    }
    // Keep the viewed component inside the available width so a vertical
    // scrollbar never causes an unnecessary horizontal scrollbar.
    content.setSize(juce::jmax(1, viewport.getMaximumVisibleWidth()), contentHeight);
}

juce::Rectangle<int> ResponsiveDialogWindow::availableArea() const
{
    const auto& displays = juce::Desktop::getInstance().getDisplays();
    const auto* display = displays.getDisplayForRect(getScreenBounds());
    if (display == nullptr)
        display = displays.getPrimaryDisplay();
    if (display == nullptr)
        return {};

    auto area = display->userArea;
    if (keyboardWasVisible && !keyboardScreenBounds.isEmpty())
    {
        const auto* focused = getFocusedEditor();
        const auto editorScreen = focused != nullptr ? focused->getScreenBounds() : getScreenBounds();
        const bool keyboardAtBottom = keyboardScreenBounds.getBottom() >= area.getBottom() - 8;
        if (keyboardAtBottom || editorScreen.getCentreY() < keyboardScreenBounds.getCentreY())
            area.setBottom(juce::jmin(area.getBottom(), keyboardScreenBounds.getY()));
        else
            area.setTop(juce::jmax(area.getY(), keyboardScreenBounds.getBottom()));
    }
    return area.reduced(16);
}

juce::Component* ResponsiveDialogWindow::getFocusedEditor() const
{
    auto* focused = juce::Component::getCurrentlyFocusedComponent();
    if (focused != nullptr && focused != this && isParentOf(focused))
        return focused;

    for (const auto& field : fields)
        if (field.component != nullptr && field.component->hasKeyboardFocus(true))
            return field.component;
    return nullptr;
}

ResponsiveDialogWindow* ResponsiveDialogWindow::getFocusedKeyboardDialog()
{
    auto& dialogs = activeKeyboardDialogs();
    auto* focused = juce::Component::getCurrentlyFocusedComponent();
    if (focused != nullptr)
        for (auto iterator = dialogs.rbegin(); iterator != dialogs.rend(); ++iterator)
            if (auto* dialog = iterator->getComponent(); dialog != nullptr && dialog->isVisible()
                && dialog->isParentOf(focused))
                return dialog;

    // A modal can be visible before JUCE transfers keyboard focus to one of
    // its editors. Route the initial inset snapshot to the topmost dialog
    // instead of dropping the only event that reports an already-open IME.
    for (auto iterator = dialogs.rbegin(); iterator != dialogs.rend(); ++iterator)
        if (auto* dialog = iterator->getComponent(); dialog != nullptr && dialog->isVisible())
            return dialog;
    return nullptr;
}

void ResponsiveDialogWindow::notifyAndroidKeyboardInset(int bottomInsetPixels)
{
    juce::MessageManager::callAsync([bottomInsetPixels]
    {
        auto* dialog = getFocusedKeyboardDialog();
        const auto* display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();
        if (dialog == nullptr || display == nullptr)
            return;

        const int inset = juce::roundToInt(static_cast<float>(bottomInsetPixels) / static_cast<float>(display->scale));
        const auto bounds = inset > 0
            ? display->totalArea.withTop(display->totalArea.getBottom() - inset).withHeight(inset)
            : juce::Rectangle<int>{};
        dialog->handleKeyboardOcclusionChanged(bounds);
    });
}

#if JUCE_WINDOWS
void ResponsiveDialogWindow::notifyWindowsKeyboardOcclusion(
    juce::Rectangle<int> physicalScreenBounds,
    juce::Component::SafePointer<ResponsiveDialogWindow> safeTarget)
{
    juce::MessageManager::callAsync([physicalScreenBounds, safeTarget]
    {
        auto* dialog = safeTarget.getComponent();
        if (dialog == nullptr || !dialog->isVisible())
            return;
        if (!physicalScreenBounds.isEmpty() && dialog->getFocusedEditor() == nullptr)
            return;

        juce::Rectangle<int> logicalBounds;
        if (!physicalScreenBounds.isEmpty())
        {
            const auto& displays = juce::Desktop::getInstance().getDisplays();
            const auto* display = displays.getDisplayForRect(physicalScreenBounds, true);
            if (display == nullptr)
                return;
            logicalBounds = displays.physicalToLogical(physicalScreenBounds, display);
        }
        dialog->handleKeyboardOcclusionChanged(logicalBounds);
    });
}
#endif

void ResponsiveDialogWindow::handleKeyboardOcclusionChanged(juce::Rectangle<int> screenBounds)
{
    if (screenBounds.isEmpty())
    {
        if (!keyboardWasVisible)
            return;
        setBounds(boundsBeforeKeyboard);
        viewport.setViewPosition(scrollPositionBeforeKeyboard);
        keyboardWasVisible = false;
        keyboardScreenBounds = {};
        if (messageVisibleAfterKeyboardDismiss && messageLabel != nullptr)
        {
            messageVisibleAfterKeyboardDismiss = false;
            messageVisible = true;
            messageLabel->setVisible(true);
            resized();

            const int desiredHeight = contentHeight + titleAreaHeight()
                + (buttons.empty() ? buttonToDialogBottomGap
                                   : contentToButtonGap + dialogButtonHeight + buttonToDialogBottomGap);
            const auto area = availableArea();
            const auto* parent = getParentComponent();
            const int availableHeight = parent != nullptr
                ? juce::jmin(area.getHeight(), parent->getHeight())
                : area.getHeight();
            if (desiredHeight <= availableHeight)
            {
                setSize(getWidth(), desiredHeight);
                if (parent != nullptr)
                    setCentrePosition(parent->getLocalBounds().getCentre());
                else
                    setCentrePosition(area.getCentre());
			}
			else
			{
				messageVisible = false;
				messageLabel->setVisible(false);
				setBounds(boundsBeforeKeyboard);
				resized();
			}
		}
		return;
    }

    if (!keyboardWasVisible)
    {
        boundsBeforeKeyboard = getBounds();
        scrollPositionBeforeKeyboard = viewport.getViewPosition();
        keyboardWasVisible = true;
    }
    keyboardScreenBounds = screenBounds;

    auto visibleArea = availableArea();
#if JUCE_ANDROID
    if (auto* parent = getParentComponent())
    {
        // Android may resize Activity content for the IME or keep it full-sized
        // with edge-to-edge. Clip only in the latter case; do not subtract the
        // same inset twice.
        auto parentArea = parent->getLocalBounds();
        const int keyboardTop = parent->getLocalPoint(nullptr, screenBounds.getTopLeft()).getY();
        const auto parentBottomOnScreen = parent->localPointToGlobal(parentArea.getBottomLeft()).getY();
        const bool parentAlreadyResized = parentBottomOnScreen <= screenBounds.getY() + 2;
        if (!parentAlreadyResized)
            parentArea.setBottom(juce::jlimit(parentArea.getY(), parentArea.getBottom(), keyboardTop));
        visibleArea = parentArea.reduced(16);
    }
#endif
    int minimumViewportHeight = 0;
    int contentY = 8;
    for (const auto& field : fields)
    {
        if (field.component == nullptr)
            continue;
        if (field.fillRemainingViewportHeight)
            minimumViewportHeight = juce::jmax(minimumViewportHeight,
                                               contentY + field.minimumHeight + field.spacingAfter + 8);
        contentY += field.fillRemainingViewportHeight ? field.minimumHeight + field.spacingAfter
                                                      : field.preferredHeight + field.spacingAfter;
    }
    const int footer = buttons.empty() ? 0 : dialogFooterHeight;
    const int minimumDialogHeight = titleAreaHeight() + footer + dialogContentBottomMargin + minimumViewportHeight;
    const int newHeight = juce::jmax(minimumDialogHeight,
                                     juce::jmin(boundsBeforeKeyboard.getHeight(), visibleArea.getHeight()));
    const int newWidth = juce::jmax(1, juce::jmin(boundsBeforeKeyboard.getWidth(), visibleArea.getWidth()));
    setSize(newWidth, newHeight);
    if (getParentComponent() != nullptr)
        setCentrePosition(visibleArea.getCentre());
    else
        setTopLeftPosition(visibleArea.getCentreX() - newWidth / 2, visibleArea.getCentreY() - newHeight / 2);

    if (auto* editor = getFocusedEditor())
    {
        const auto editorBounds = content.getLocalArea(editor, editor->getLocalBounds());
        const int visibleTop = viewport.getViewPositionY();
        const int visibleBottom = visibleTop + viewport.getHeight();
        if (editorBounds.getBottom() > visibleBottom)
            viewport.setViewPosition(0, editorBounds.getBottom() - viewport.getHeight());
        else if (editorBounds.getY() < visibleTop)
            viewport.setViewPosition(0, editorBounds.getY());
    }
}

#if JUCE_ANDROID
extern "C" JNIEXPORT void JNICALL
Java_com_openagriculture_agisovirtualterminal_MainActivity_onImeInsetsChanged(JNIEnv*, jclass, jint bottomInsetPixels)
{
    ResponsiveDialogWindow::notifyAndroidKeyboardInset(bottomInsetPixels);
}
#endif
