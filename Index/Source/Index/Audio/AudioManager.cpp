#include "Precompiled.h"
#include "AudioManager.h"
#include "Core/Application.h"

#include "Scene/Component/SoundComponent.h"

#ifdef INDEX_OPENAL
#include "Platform/OpenAL/ALManager.h"
#endif

namespace Index
{
    AudioManager* AudioManager::Create()
    {
#ifdef INDEX_OPENAL
        return new Audio::ALManager();
#else
        return nullptr;
#endif
    }

    void AudioManager::SetPaused(bool paused)
    {
        m_Paused = paused;
        if(!Application::Get().GetCurrentScene())
            return;
        auto soundsView = Application::Get().GetCurrentScene()->GetRegistry().view<SoundComponent>();

        if(m_Paused)
        {
            for(auto entity : soundsView)
                soundsView.get<SoundComponent>(entity).GetSoundNode()->Stop();
        }
        else
        {
            for(auto entity : soundsView)
            {
                auto soundNode = soundsView.get<SoundComponent>(entity).GetSoundNode();
                if(!soundNode->GetPaused())
                    soundNode->Resume();
            }
        }
    }
}
