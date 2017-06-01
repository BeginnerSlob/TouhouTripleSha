#ifndef AUDIO_H
#define AUDIO_H

#ifdef AUDIO_SUPPORT

class Audio
{
public:
    static void init();
    static void quit();

    static void play(const QString &fileName, bool continuePlayWhenPlaying = false);

    static void setEffectVolume(float volume);
    static void setBackgroundMusicVolume(float volume);

    static void playBackgroundMusic(const QString &fileNames, bool random = false);
    static void stopBackgroundMusic();
    static bool isBackgroundMusicPlaying();

    static const QString &getCustomBackgroundMusicFileName()
    {
        return m_customBackgroundMusicFileName;
    }
    static void setCustomBackgroundMusicFileName(const QString &customBackgroundMusicFileName)
    {
        m_customBackgroundMusicFileName = customBackgroundMusicFileName;
    }
    static void resetCustomBackgroundMusicFileName()
    {
        m_customBackgroundMusicFileName.clear();
    }

    static void stopAll();

    static QString getVersion();

private:
    static QString m_customBackgroundMusicFileName;
    static const int MAX_CHANNEL_COUNT = 100;
};

#endif // AUDIO_SUPPORT

#endif
