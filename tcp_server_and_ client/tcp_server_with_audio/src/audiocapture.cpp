#include "audiocapture.h"

//构造
AudioCapture::AudioCapture()
{

}

//析构
AudioCapture::~AudioCapture()
{
	
}

int AudioCapture::initAudioCapture(audio_pcm_t *audio) {
    int rc;
   m_pAudioPcm = audio;
    if (m_pAudioPcm == NULL) {
        fprintf(stderr, "input audio is NULL\n");
        return -1;
    }

    // open PCM device for recording(captuer)
    rc = snd_pcm_open(&(m_pAudioPcm->handle), "default", SND_PCM_STREAM_CAPTURE, 0);
    if (rc < 0) {
        fprintf(stderr, "unable to opren pcm device: %s\n", snd_strerror(rc));
        return -1;
    }

    // Allocate a hardware parameters object
    snd_pcm_hw_params_alloca(&(m_pAudioPcm->params));

    // use default values 
    snd_pcm_hw_params_any(m_pAudioPcm->handle, m_pAudioPcm->params);

    // set the desired hw parameters
    // Interleaced mode
    snd_pcm_hw_params_set_access(m_pAudioPcm->handle, m_pAudioPcm->params, SND_PCM_ACCESS_RW_INTERLEAVED);

    // signed 16bit little-endian format
    snd_pcm_hw_params_set_format(m_pAudioPcm->handle, m_pAudioPcm->params, SND_PCM_FORMAT_S16_LE);

    // set double channels
    snd_pcm_hw_params_set_channels(m_pAudioPcm->handle, m_pAudioPcm->params, m_pAudioPcm->channels);

    // 44100 bits/second sampliing rate(CD quality)
    snd_pcm_hw_params_set_rate_near(m_pAudioPcm->handle, m_pAudioPcm->params, &(m_pAudioPcm->val), &(m_pAudioPcm->dir));

    // set period size to 32 frames
    snd_pcm_hw_params_set_period_size_near(m_pAudioPcm->handle, m_pAudioPcm->params, &(m_pAudioPcm->frames), &(m_pAudioPcm->dir));

    // write the parameters to the device
    rc = snd_pcm_hw_params(m_pAudioPcm->handle, m_pAudioPcm->params);
    if (rc < 0) {
        fprintf(stderr, "unable to set hw parameters: %s\n", snd_strerror(rc));
        return -1;
    }

    // use a buffer large enough to hold one period
    snd_pcm_hw_params_get_period_size(m_pAudioPcm->params, &(m_pAudioPcm->frames), &(m_pAudioPcm->dir));

    // we want loop for 5s
    snd_pcm_hw_params_get_period_time(m_pAudioPcm->params, &(m_pAudioPcm->val), &(m_pAudioPcm->dir));

    m_nPcmBufSize  = (m_pAudioPcm->frames)*4;
	m_pPcmBuffer =  (char *)malloc(m_nPcmBufSize*2);
 
    return 0;
}

int AudioCapture::captureAudio() {
    int rc;

    if (m_pAudioPcm == NULL || m_pPcmBuffer == NULL)
    {
        fprintf(stderr, "input audio is NULL\n");
        return -1;
    }

    // get device data
    rc = snd_pcm_readi(m_pAudioPcm->handle, m_pPcmBuffer, m_pAudioPcm->frames);
    if (rc == -EPIPE)
    {
        // EPIPE means overrun
        fprintf(stderr, "overrun occurred\n");
        snd_pcm_prepare(m_pAudioPcm->handle);
        return -1;
    } 
    else if (rc < 0)
    {
        fprintf(stderr, "read error %s\n", snd_strerror(rc));
        return -1;
    } 
    else if (rc != (int) m_pAudioPcm->frames) 
    {
        fprintf(stderr, "short read, raed %d frames\n", rc);
        return -1;
    }
    fprintf(stderr, "captureAudio %d frames\n", rc);
    return rc;
}

int AudioCapture:: closeCaptureDevice() {
    if (m_pAudioPcm == NULL) {
        fprintf(stderr, "input audio is NULL\n");
        return -1;
    }
    if(m_pAudioPcm->handle != NULL)
   {
      snd_pcm_drain(m_pAudioPcm->handle);
      snd_pcm_close(m_pAudioPcm->handle);
      return 0;
   }
    return 0;
}
