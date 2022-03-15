#pragma once

#include <JuceHeader.h>

//https://github.com/ffAudio/foleys_gui_magic/blob/main/Helpers/foleys_ParameterAttachment.h#L43

template<typename ValueType>
class ParamAttachment : private juce::AudioProcessorParameter::Listener,
                            private juce::AsyncUpdater
{
public:
    ParamAttachment()
    {
    }

    virtual ~ParamAttachment() override
    {
        detachFromParameter();
    }

    /**
     Thread safe way to read the current value.
     */
    ValueType getValue() const
    {
        return value.load();
    }

    /**
     Thread safe way to read the normalised value of the current value.
     */
    ValueType getNormalisedValue() const
    {
        if (parameter)
            return parameter->getNormalisableRange().convertTo0to1 (value.load());

        return value.load();
    }

    /**
     Set the value from a not normalised range.min..range.max value.
     */
    void setValue (ValueType newValue)
    {
        if (parameter)
            parameter->setValueNotifyingHost (parameter->getNormalisableRange().convertTo0to1 (newValue));
        else
            parameterValueChanged (0, juce::jlimit (0.0f, 1.0f, newValue));
    }

    /**
     Set the value from a normalised 0..1 value.
     */
    void setNormalisedValue (ValueType newValue)
    {
        if (parameter)
            parameter->setValueNotifyingHost (newValue);
        else
            parameterValueChanged (0, juce::jlimit (0.0f, 1.0f, newValue));
    }

    /**
     Make this value attached to the parameter with the supplied parameterID.
     */
    void attachToParameter (juce::RangedAudioParameter* parameterToUse)
    {
        detachFromParameter();

        if (parameterToUse)
        {
            parameter = parameterToUse;

            initialUpdate();

            parameter->addListener (this);
        }
    }

    void detachFromParameter()
    {
        if (parameter)
            parameter->removeListener (this);
    }

    /**
     Make sure to call this before you send changes (e.g. from mouseDown of your UI widget),
     otherwise the hosts automation will battle with your value changes.
     */
    void beginGesture()
    {
        if (parameter)
            parameter->beginChangeGesture();
    }

    /**
     Make sure to call this after finishing your changes (e.g. from mouseDown of your UI widget),
     this way the automation can take back control (like e.g. latch mode).
     */
    void endGesture()
    {
        if (parameter)
            parameter->endChangeGesture();
    }

    void parameterValueChanged (int parameterIndex, float newValue) override
    {
        juce::ignoreUnused (parameterIndex);
        if (parameter)
            value.store (ValueType (parameter->convertFrom0to1 (newValue)));
        else
            value.store (ValueType (newValue));

        if (onParameterChanged)
            onParameterChanged();

        if (onParameterChangedAsync)
            triggerAsyncUpdate();
    }

    void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override
    { juce::ignoreUnused (parameterIndex); juce::ignoreUnused (gestureIsStarting); }

    void handleAsyncUpdate() override
    {
        if (onParameterChangedAsync)
            onParameterChangedAsync();
    }

    /**
     Set this lambda to be called from whatever thread is updating the parameter
     */
    std::function<void()> onParameterChanged;

    /**
     Set this lambda to be called from the message thread via AsyncUpdater
     */
    std::function<void()> onParameterChangedAsync;

private:

    void initialUpdate()
    {
        if (parameter)
            value.store (ValueType (parameter->convertFrom0to1 (parameter->getValue())));
        else
            value.store (ValueType());

        if (onParameterChanged)
            onParameterChanged();
    }

    juce::RangedAudioParameter*         parameter = nullptr;
    std::atomic<ValueType>              value { ValueType() };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParamAttachment)
};