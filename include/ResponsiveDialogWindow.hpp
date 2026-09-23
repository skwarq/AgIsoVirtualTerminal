#pragma once

#include <JuceHeader.h>

class CenteredPopupLookAndFeel : public juce::LookAndFeel_V4
{
public:
	void drawPopupMenuItem(juce::Graphics&, const juce::Rectangle<int>&, bool, bool, bool, bool, bool,
	                       const juce::String&, const juce::String&, const juce::Drawable*, const juce::Colour*) override;
};

/// A reusable modal dialog with an independent content area and button bar.
/// The dialog owns its layout and never delegates geometry to AlertWindow.
class ResponsiveDialogWindow : public juce::Component
{
public:
	static constexpr int dialogHorizontalMargin = 16;
	static constexpr int dialogTitleHeight = 40;
	static constexpr int dialogFooterHeight = 60;
	static constexpr int dialogContentBottomMargin = 6;
	static constexpr int dialogFieldHeight = 30;
	static constexpr int dialogFieldSpacing = 12;
	static constexpr int contentToButtonGap = 16;
    static constexpr int dialogButtonHeight = 34;
    static constexpr int buttonToDialogBottomGap = 16;

    ResponsiveDialogWindow(juce::String title, juce::String message = {});
    ~ResponsiveDialogWindow() override;

    void addTextBlock(const juce::String& text);
    juce::TextEditor* addTextEditor(const juce::String& key,
                                    const juce::String& value,
                                    const juce::String& label = {});
    juce::ComboBox* addComboBox(const juce::String& key,
                                const juce::StringArray& choices,
                                const juce::String& label = {});
    juce::TextButton* addButton(const juce::String& name, int result,
                                std::function<bool()> canClose = {});
    void addCustomComponent(juce::Component* component, int preferredHeight = 34, int spacingAfter = 12);
    void setInfoIconVisible(bool visible);
    void setVerticalScrollingEnabled(bool enabled);

    juce::String getTextEditorContents(const juce::String& key) const;
    juce::TextEditor* getTextEditor(const juce::String& key) const;
    juce::ComboBox* getComboBoxComponent(const juce::String& key) const;

    void showModal(juce::Component& parent, std::function<void(int)> callback);
    void close(int result);
    void resized() override;
    void paint(juce::Graphics&) override;

private:
    struct Field
    {
        juce::String key;
        juce::String label;
        juce::Component* component = nullptr;
        juce::Label* labelComponent = nullptr;
        int preferredHeight = 34;
        int spacingAfter = 12;
    };

    void layoutContent();
    juce::Rectangle<int> availableArea() const;

    juce::String title;
    juce::String message;
    std::unique_ptr<juce::TextEditor> messageLabel;
    juce::Viewport viewport;
    juce::Component content;
    juce::Component buttonBar;
    std::vector<Field> fields;
    std::vector<std::unique_ptr<juce::Component>> ownedComponents;
    struct DialogButton { juce::TextButton* button; int result; std::function<bool()> canClose; };
    std::vector<DialogButton> buttons;
    std::function<void(int)> callback;
    CenteredPopupLookAndFeel centeredPopupLookAndFeel;
    int contentHeight = 0;
    bool infoIconVisible = false;
    bool verticalScrollVisible = false;
    bool verticalScrollingEnabled = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ResponsiveDialogWindow)
};
