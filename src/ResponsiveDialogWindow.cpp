#include "ResponsiveDialogWindow.hpp"

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

void ResponsiveDialogWindow::addCustomComponent(juce::Component* component, int preferredHeight, int spacingAfter)
{
    if (component == nullptr) return;
    content.addAndMakeVisible(*component);
    fields.push_back({ "custom", {}, component, nullptr, preferredHeight, spacingAfter });
}

void ResponsiveDialogWindow::setInfoIconVisible(bool visible)
{
    infoIconVisible = visible;
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
#if JUCE_ANDROID
    // Keep the dialog in the host Activity. A desktop peer becomes a separate
    // Android Activity and closing it can background the host application.
    parent.addAndMakeVisible(*this);
#else
    addToDesktop(juce::ComponentPeer::windowIsTemporary);
#endif
    setVisible(true);
    // Keep dialogs comfortably readable even when their content is short;
    // the measured content still determines the final size above this minimum.
    int desiredWidth = 320;
    desiredWidth = juce::jmax(desiredWidth, measureTextWidth(title, 16.0f) + 48);
    for (const auto& line : juce::StringArray::fromLines(message))
        desiredWidth = juce::jmax(desiredWidth, measureTextWidth(line, 16.0f) + 48);
    for (const auto& field : fields)
    {
        desiredWidth = juce::jmax(desiredWidth, measureTextWidth(field.label, 16.0f) + 48);
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
    const int desiredHeight = contentHeight + 40
        + (buttons.empty() ? buttonToDialogBottomGap
                           : contentToButtonGap + dialogButtonHeight + buttonToDialogBottomGap);
    const int fittedHeight = juce::jmin(desiredHeight, display.getHeight());
    setSize(fittedWidth, fittedHeight);
#if JUCE_ANDROID
    setCentrePosition(parent.getLocalBounds().getCentre());
#else
    setCentrePosition(display.getCentreX(), display.getCentreY());
#endif
    enterModalState(true);
}

void ResponsiveDialogWindow::close(int result)
{
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
    viewport.setBounds(dialogHorizontalMargin, dialogTitleHeight, getWidth() - 2 * dialogHorizontalMargin, getHeight() - footer - dialogTitleHeight - dialogContentBottomMargin);
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
    if (messageLabel != nullptr)
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
            field.component->setBounds(0, y, juce::jmax(1, viewport.getWidth() - 24), field.preferredHeight);
            y += field.preferredHeight + field.spacingAfter;
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
    return juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->userArea.reduced(16);
}
