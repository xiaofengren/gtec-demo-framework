/****************************************************************************************************************************************************
* Copyright (c) 2014 Freescale Semiconductor, Inc.
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
*    * Redistributions of source code must retain the above copyright notice,
*      this list of conditions and the following disclaimer.
*
*    * Redistributions in binary form must reproduce the above copyright notice,
*      this list of conditions and the following disclaimer in the documentation
*      and/or other materials provided with the distribution.
*
*    * Neither the name of the Freescale Semiconductor, Inc. nor the names of
*      its contributors may be used to endorse or promote products derived from
*      this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
* BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
* OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
****************************************************************************************************************************************************/

#include <FslBase/Exceptions.hpp>
#include <FslBase/Log/Log.hpp>
#include <FslDemoHost/Base/Service/Image/IImageServiceControl.hpp>
#include <FslDemoApp/Base/Host/DemoHostFeatureUtil.hpp>
#include <FslDemoHost/Base/Service/WindowHost/IWindowHostInfoControl.hpp>
#include <FslDemoHost/EGL/EGLDemoHost.hpp>
#include <FslDemoHost/EGL/EGLDemoHostOptionParser.hpp>
#include <FslNativeWindow/EGL/EGLNativeWindowSystemFactory.hpp>
#include <FslNativeWindow/EGL/IEGLNativeWindowSystem.hpp>
#include <FslNativeWindow/EGL/IEGLNativeWindow.hpp>
#include <FslDemoApp/Base/Host/DemoAppHostConfigEGL.hpp>
#include <FslNativeWindow/Base/NativeWindowEventQueue.hpp>
#include <FslNativeWindow/Base/NativeWindowProcessMessagesArgs.hpp>
#include <FslNativeWindow/Base/NativeWindowSystemSetup.hpp>
#include <FslUtil/EGL/EGLCheck.hpp>
#include <FslUtil/EGL/Exceptions.hpp>
#include <FslUtil/EGL/EGLStringUtil.hpp>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <vector>
//#include <EGL/eglext.h>
#include "Service/EGLHost/EGLHostService.hpp"

#if 0
#define LOCAL_LOG(X) FSLLOG("EGLDemoHost: " << X)
#else
#define LOCAL_LOG(X) {}
#endif

#if 1
#define LOCAL_LOG_WARNING(X) FSLLOG_WARNING("EGLDemoHost: " << X)
#else
#define LOCAL_LOG_WARNING(X) {}
#endif

#define EMPTY_VALUE_EGLCONFIG nullptr

namespace Fsl
{
  namespace
  {
    struct RGBConfig
    {
      int32_t R;
      int32_t G;
      int32_t B;

      RGBConfig()
        : R(8)
        , G(8)
        , B(8)
      {
      }

      RGBConfig(const int32_t r, const int32_t g, const int32_t b)
        : R(r)
        , G(g)
        , B(b)
      {
      }
    };

    const int32_t DEFAULT_DEPTH_BUFFER_SIZE = 24;
    const int32_t FALLBACK_DEPTH_BUFFER_SIZE = 16;
    const RGBConfig DEFAULT_RGB_CONFIG(8,8,8);



    //! @brief copy the supplied EGL config into a deque
    void CopyConfig(std::deque<EGLint>& rEglConfigAttribs, const int32_t* pEglConfigAttribs)
    {
      rEglConfigAttribs.clear();
      if (pEglConfigAttribs != nullptr)
      {
        const int MAX = 1024;
        int count = 0;
        while (*pEglConfigAttribs != EGL_NONE && count < MAX)
        {
          rEglConfigAttribs.push_back(*pEglConfigAttribs);
          ++pEglConfigAttribs;
          ++count;
        }
        if (*pEglConfigAttribs != EGL_NONE)
          throw std::invalid_argument("pEglConfigAttribs seems to be invalid");
        else
          rEglConfigAttribs.push_back(*pEglConfigAttribs);
      }
    }


    void BuildDefaultConfigAttributes(std::deque<EGLint>& rConfigAttributes, const EGLDemoHostFeatureConfig& featureConfig, const bool bForceConformance, const RGBConfig& preferredRGBConfig, const int32_t preferredDepthBufferSize)
    {
      rConfigAttributes.push_back(EGL_RED_SIZE);
      rConfigAttributes.push_back(preferredRGBConfig.R);

      rConfigAttributes.push_back(EGL_GREEN_SIZE);
      rConfigAttributes.push_back(preferredRGBConfig.G);

      rConfigAttributes.push_back(EGL_BLUE_SIZE);
      rConfigAttributes.push_back(preferredRGBConfig.B);

      if (featureConfig.EnableGLES)
      {
        switch (featureConfig.ESVersionMajor)
        {
        case 2:
          rConfigAttributes.push_back(EGL_RENDERABLE_TYPE);
          rConfigAttributes.push_back(EGL_OPENGL_ES2_BIT);
          break;
        case 3:
#ifdef EGL_OPENGL_ES3_BIT
          rConfigAttributes.push_back(EGL_RENDERABLE_TYPE);
          rConfigAttributes.push_back(EGL_OPENGL_ES3_BIT);
#elif defined(EGL_OPENGL_ES3_BIT_KHR)
          rConfigAttributes.push_back(EGL_RENDERABLE_TYPE);
          rConfigAttributes.push_back(EGL_OPENGL_ES3_BIT_KHR);
#endif
          break;
        default:
          throw NotSupportedException("Unknown OpenGLES version request");
        }
        rConfigAttributes.push_back(EGL_DEPTH_SIZE);
        rConfigAttributes.push_back(preferredDepthBufferSize);
      }

      if (featureConfig.EnableVG)
      {
        rConfigAttributes.push_back(EGL_RENDERABLE_TYPE);
        rConfigAttributes.push_back(EGL_OPENVG_BIT);

        //  rConfigAttributes.push_back(EGL_ALPHA_SIZE);
        //  rConfigAttributes.push_back(8);
      }

      //if (bUseOpenGLES && bForceConformance)
      //{
      //  rConfigAttributes.push_back(EGL_CONFORMANT);
      //  rConfigAttributes.push_back(EGL_OPENGL_ES2_BIT);
      //}

      // While the default ought to be  EGL_SURFACE_TYPE = EGL_WINDOW_BIT
      // The arm mali 1.3.2 and 1.4 emulator fails to render correctly without this on newer nvidia cards.
      rConfigAttributes.push_back(EGL_SURFACE_TYPE);
      rConfigAttributes.push_back(EGL_WINDOW_BIT);

      rConfigAttributes.push_back(EGL_SAMPLES);
      rConfigAttributes.push_back(0);

      rConfigAttributes.push_back(EGL_NONE);
    }


    const Point2 GetEGLSurfaceResolution(EGLDisplay hEglDisplay, EGLSurface hEglSurface)
    {
      EGLint windowWidth;
      EGLint windowHeight;
      eglQuerySurface(hEglDisplay, hEglSurface, EGL_WIDTH, &windowWidth);
      eglQuerySurface(hEglDisplay, hEglSurface, EGL_HEIGHT, &windowHeight);
      return Point2(windowWidth, windowHeight);
    }


    //! @brief replace the attribute if its found
    //! @return true if replaced, false if not found
    bool ReplaceAttribute(std::deque<EGLint>& rConfigAttributes, const EGLint key, const EGLint value)
    {
      assert((rConfigAttributes.size() % 2) == 1 && rConfigAttributes.back() == EGL_NONE);

      std::deque<EGLint>::iterator itr = rConfigAttributes.begin();
      int index = 0;
      while (itr != rConfigAttributes.end() && *itr != EGL_NONE)
      {
        if ((index & 1) == 0 && *itr == key)
        {
          ++itr;
          *itr = value;
          return true;
        }
        ++itr;
        ++index;
      }
      return false;
    }

    void MergeWithOverwriteConfigAttributes(std::deque<EGLint>& rConfigAttributes, const std::deque<EGLint>& srcConfigAttribs)
    {
      if (srcConfigAttribs.size() == 0)
        return;

      assert((rConfigAttributes.size() % 2) == 1);
      assert((srcConfigAttribs.size() % 2) == 1);

      std::deque<EGLint>::const_iterator itr = srcConfigAttribs.begin();
      while (itr != srcConfigAttribs.end() && *itr != EGL_NONE)
      {
        EGLint key = *itr;
        ++itr;
        EGLint value = *itr;
        ++itr;

        if (!ReplaceAttribute(rConfigAttributes, key, value))
        {
          rConfigAttributes.pop_back();
          rConfigAttributes.push_back(key);
          rConfigAttributes.push_back(value);
          rConfigAttributes.push_back(EGL_NONE);
        }
      }

    }

    void ApplyHostOverrides(std::deque<EGLint>& rConfigAttributes, const std::shared_ptr<EGLDemoHostOptionParser> options)
    {
      std::deque<EGLint> forceConfigAttributes;
      options->ExtractConfigAttributes(forceConfigAttributes);
      if (forceConfigAttributes.size() > 0)
        MergeWithOverwriteConfigAttributes(rConfigAttributes, forceConfigAttributes);
    }

    EGLDemoHostFeatureConfig ExamineFeatureRequest(const std::shared_ptr<DemoHostAppSetup::DemoHostFeatureDeque>& features)
    {
      EGLDemoHostFeatureConfig featureConfig;
      DemoHostAppSetup::DemoHostFeatureDeque::const_iterator itr = features->begin();
      while (itr != features->end())
      {
        if (itr->Name == DemoHostFeatureName::OpenGLES)
        {
          featureConfig.EnableGLES = true;

          int major, minor;
          DemoHostFeatureUtil::DecodeOpenGLESVersion(itr->Version, major, minor);
          featureConfig.ESVersionMajor = major;
          featureConfig.ESVersionMinor = minor;
          featureConfig.Feature = *itr;
        }
        else if (itr->Name == DemoHostFeatureName::OpenVG)
        {
          featureConfig.EnableVG = true;

          int major, minor;
          DemoHostFeatureUtil::DecodeOpenVGVersion(itr->Version, major, minor);
          featureConfig.VGVersion = major;
          featureConfig.Feature = *itr;
        }
        ++itr;
      }
      return featureConfig;
    }

    void ExamineFinalConfig(const std::vector<EGLint>& finalConfigAttribs)
    {
      // start at 1 so that the size check is simpler
      for (std::size_t i = 1; i < finalConfigAttribs.size(); i += 2)
      {
        switch (finalConfigAttribs[i - 1])
        {
        case EGL_RED_SIZE:
        case EGL_GREEN_SIZE:
        case EGL_BLUE_SIZE:
        case EGL_ALPHA_SIZE:
          if (finalConfigAttribs[i] < 0)
          {
            FSLLOG_WARNING(EGLStringUtil::GetConfigEnumToString(finalConfigAttribs[i - 1]) << " can not be negative.");
          }
          break;
        }
      }
    }

    void DoCheckExtensions(const EGLDisplay display, const std::deque<ExtensionRequestRecord>& extensionRequests)
    {
      const auto extensions = EGLStringUtil::GetExtensions(display);
      for (const auto& request : extensionRequests)
      {
        if (request.Type == ExtensionType::EGL && std::find(extensions.begin(), extensions.end(), request.Name) == extensions.end())
        {
          // Request not found, lets check how severe it is
          switch (request.Precense)
          {
          case ExtensionPrecense::Mandatory:
            throw std::runtime_error(std::string("Required EGL extension '") + request.Name + "' not found");
          case ExtensionPrecense::Optional:
            FSLLOG_DEBUG("Optional EGL extension '" << request.Name << "' not available.");
            break;
          default:
            throw NotSupportedException("Unsupported ExtensionPrecense");
          }
        }
      }
    }


    void DoLogExtensions(const EGLDisplay display)
    {
      const auto extensions = EGLStringUtil::GetExtensions(display);
      FSLLOG("EGL Extensions");
      for (const auto& entry : extensions)
      {
        FSLLOG("- " << entry);
      }
    }


    void DoLogConfig(const std::vector<EGLint>& finalConfigAttribs)
    {
      // start at 1 so that the size check is simpler
      FSLLOG("Requested EGL config");
      for (std::size_t i = 1; i < finalConfigAttribs.size(); i += 2)
      {
        FSLLOG(EGLStringUtil::GetConfigEnumToString(finalConfigAttribs[i - 1]) << "=" << finalConfigAttribs[i]);
      }
    }

    void DoLogConfigComparison(EGLDisplay hDisplay, EGLConfig config, const std::vector<EGLint>& finalConfigAttribs)
    {
      FSLLOG("Final vs requested EGL config");

      for (std::size_t i = 1; i < finalConfigAttribs.size(); i += 2)
      {
        const EGLint attrib = finalConfigAttribs[i - 1];
        const EGLint requestedValue = finalConfigAttribs[i];
        EGLint actualValue;
        EGL_CHECK(eglGetConfigAttrib(hDisplay, config, attrib, &actualValue));

        if (actualValue != requestedValue)
        {
          FSLLOG(EGLStringUtil::GetConfigEnumToString(finalConfigAttribs[i - 1]) << " is " << actualValue << " requested " << requestedValue);
        }
      }
    }


    void BuildAppEGLConfig(std::deque<EGLint>& rAppConfigAttribs, const int32_t* appHostConfigEglConfigAttribs)
    {
      // Do a basic validation on the app's custom EGL config attribs
      CopyConfig(rAppConfigAttribs, appHostConfigEglConfigAttribs);

      if (!(rAppConfigAttribs.size() == 0 || ((rAppConfigAttribs.size() % 2) == 1 && rAppConfigAttribs.back() == EGL_NONE)))
        throw std::invalid_argument("eglConfigAttribs is in a unexpected format");
    }


    void BuildEGLConfig(std::vector<EGLint>& rFinalConfigAttribs, const std::deque<EGLint>& appAglConfigAttribs, const ConfigControl configControl, const EGLDemoHostFeatureConfig& featureConfig, const std::shared_ptr<EGLDemoHostOptionParser>& options, const RGBConfig& preferredRGBConfig, const int32_t preferredDepthBufferSize)
    {
      // Then build our default egl config
      std::deque<EGLint> configAttribs;
      if (appAglConfigAttribs.size() <= 0 || configControl == ConfigControl::Overwrite)
        BuildDefaultConfigAttributes(configAttribs, featureConfig, false, preferredRGBConfig, preferredDepthBufferSize);
      if (appAglConfigAttribs.size() > 0)
        MergeWithOverwriteConfigAttributes(configAttribs, appAglConfigAttribs);

      // Allow the command line options to override things in the config
      ApplyHostOverrides(configAttribs, options);

      // Store the final configuration for later use
      rFinalConfigAttribs.resize(configAttribs.size());
      std::copy(configAttribs.begin(), configAttribs.end(), rFinalConfigAttribs.begin());

      ExamineFinalConfig(rFinalConfigAttribs);
    }



    EGLContext DoEGLCreateContext(EGLDisplay hDisplay, EGLConfig hConfig, const bool enableGLES, const int eglContextClientVersionMajor, const int eglContextClientVersionMinor, const int eglContextClientVersionMinimumMinor)
    {
      EGLint contextAttribListESMajorOnly[] = { EGL_CONTEXT_CLIENT_VERSION, eglContextClientVersionMajor, EGL_NONE };
      bool supportsMinorVersion = true;
#if defined(EGL_CONTEXT_MAJOR_VERSION) && defined(EGL_CONTEXT_MINOR_VERSION)
      EGLint contextAttribListESMajorMinor[] = { EGL_CONTEXT_MAJOR_VERSION, eglContextClientVersionMajor, EGL_CONTEXT_MINOR_VERSION, eglContextClientVersionMinor, EGL_NONE };
      EGLint* contextAttribListES = (eglContextClientVersionMajor < 3 || eglContextClientVersionMinor == 0) ? contextAttribListESMajorOnly : contextAttribListESMajorMinor;
#elif defined(EGL_CONTEXT_MAJOR_VERSION_KHR) && defined(EGL_CONTEXT_MINOR_VERSION_KHR)
      EGLint contextAttribListESMajorMinor[] = { EGL_CONTEXT_MAJOR_VERSION_KHR, eglContextClientVersionMajor, EGL_CONTEXT_MINOR_VERSION_KHR, eglContextClientVersionMinor, EGL_NONE };
      EGLint* contextAttribListES = (eglContextClientVersionMajor < 3 || eglContextClientVersionMinor == 0) ? contextAttribListESMajorOnly : contextAttribListESMajorMinor;
#else
      EGLint* contextAttribListES = contextAttribListESMajorOnly;
      FSLLOG_WARNING_IF(eglContextClientVersionMinor != 0, "EGL does not supported requesting a minor version. Hoping for the best");
      supportsMinorVersion = false;
#endif
      EGLint contextAttribListVG[] = { EGL_NONE };
      EGLint* contextAttribList = (enableGLES ? contextAttribListES : contextAttribListVG);

      EGLContext hContext = eglCreateContext(hDisplay, hConfig, EGL_NO_CONTEXT, contextAttribList);
      if (hContext == EGL_NO_CONTEXT)
      {
        // Get the error and thereby clearing the error flag
        const EGLint eglError = eglGetError();

        if (supportsMinorVersion && eglContextClientVersionMinor > eglContextClientVersionMinimumMinor)
        {
          // Recursive try a lesser minor version until we succeed or hit minor version zero
          hContext = DoEGLCreateContext(hDisplay, hConfig, enableGLES, eglContextClientVersionMajor, eglContextClientVersionMinor - 1, eglContextClientVersionMinimumMinor);
        }
        else if (eglError != EGL_SUCCESS)
        {
          std::stringstream stream;
          stream << "Failed to create the requested context with error code " << eglError << " at " << __FILE__ << "(" << __LINE__ << ")";
          throw EGLGraphicsException(stream.str(), eglError, __FILE__, __LINE__);
        }
        else
          throw EGLGraphicsException("Failed to create the requested context");
      }
      EGL_CHECK_FOR_ERROR();
      return hContext;
    }
  }


  EGLDemoHost::EGLDemoHost(const DemoHostConfig& demoHostConfig)
    : ADemoHost(demoHostConfig)
    , m_demoHostConfig(demoHostConfig)
    , m_finalConfigAttribs()
    , m_eglContextClientVersionMajor(0)
    , m_eglContextClientVersionMinor(0)
    , m_eglContextClientVersionMinimumMinor(0)
    , m_nativeWindowSetup()
    , m_windowSystem()
    , m_window()
    , m_windowHostInfoControl(demoHostConfig.GetServiceProvider().Get<IWindowHostInfoControl>())
    , m_hostService(demoHostConfig.GetServiceProvider().Get<EGLHostService>())
    , m_hDisplay(EGL_NO_DISPLAY)
    , m_hSurface(EGL_NO_SURFACE)
    , m_hContext(EGL_NO_CONTEXT)
    , m_hConfig(EMPTY_VALUE_EGLCONFIG)
    , m_isActivated(true)
    , m_options(demoHostConfig.GetOptions<EGLDemoHostOptionParser>())
    , m_configControl(ConfigControl::Default)
    , m_featureConfig()
    , m_enableGLES(false)
    , m_enableVG(false)
    , m_logSelectedConfig(false)
    , m_logExtensions(false)
    , m_apiInit(false)
    , m_activeApi(DemoHostFeatureName::OpenGLES, 0)
  {
    const NativeWindowSystemSetup nativeWindowSystemSetup(demoHostConfig.GetEventQueue(), m_demoHostConfig.GetVerbosityLevel(),
                                                          m_options->GetNativeWindowConfig(), m_options->GetNativeWindowTag());

    m_windowSystem = EGLNativeWindowSystemFactory::Allocate(nativeWindowSystemSetup);
    // Set the window system in the host service so that any services or app that is interested will be able to access it
    m_windowHostInfoControl->SetWindowSystem(m_windowSystem);

    const DemoHostAppSetup hostAppSetup = demoHostConfig.GetDemoHostAppSetup();
    // Retrieve the custom config
    const auto appHostConfig = hostAppSetup.GetDemoAppHostConfig<DemoAppHostConfigEGL>();

    m_configControl = appHostConfig->GetConfigControl();

    m_extensionRequests = appHostConfig->GetExtensionRequests();

    // Check that its a OpenGLES demo app
    m_featureConfig = ExamineFeatureRequest(hostAppSetup.DemoHostFeatures);
    if (!m_featureConfig.EnableGLES && !m_featureConfig.EnableVG)
      throw UsageErrorException("The EGLDemo host currently expects that OpenGLES or OpenVG has to be enabled");

    m_activeApi = m_featureConfig.Feature;
    m_enableGLES = m_featureConfig.EnableGLES;
    m_enableVG = m_featureConfig.EnableVG;

    m_eglContextClientVersionMajor = m_featureConfig.ESVersionMajor;
    m_eglContextClientVersionMinor = m_featureConfig.ESVersionMinor;
    if (! appHostConfig->IsMinimumMiniorVersionSet())
      m_eglContextClientVersionMinimumMinor = m_featureConfig.ESVersionMinor;
    else
      m_eglContextClientVersionMinimumMinor = std::min(m_featureConfig.ESVersionMinor, appHostConfig->GetMinimumMiniorVersion());

    if (m_enableGLES)
    {
      std::shared_ptr<IImageServiceControl> imageControl = demoHostConfig.GetServiceProvider().Get<IImageServiceControl>();
      imageControl->SetPreferredBitmapOrigin(BitmapOrigin::LowerLeft);
    }

    BuildAppEGLConfig(m_appAglConfigAttribs, appHostConfig->GetEglConfigAttribs());
    BuildEGLConfig(m_finalConfigAttribs, m_appAglConfigAttribs, m_configControl, m_featureConfig, m_options, DEFAULT_RGB_CONFIG, DEFAULT_DEPTH_BUFFER_SIZE);

    if (m_options->IsLogConfigEnabled())
    {
      DoLogConfig(m_finalConfigAttribs);
      m_logSelectedConfig = true;
    }
    m_logExtensions = m_options->IsLogExtensionsEnabled();

    // Prepare the native window setup
    m_nativeWindowSetup.reset(new NativeWindowSetup(demoHostConfig.GetDemoHostAppSetup().AppSetup.ApplicationName, demoHostConfig.GetEventQueue(),
                                                    m_options->GetNativeWindowConfig(), m_demoHostConfig.GetVerbosityLevel()));
  }


  EGLDemoHost::~EGLDemoHost()
  {
    try
    {
      m_windowHostInfoControl->ClearWindowSystem();
      LOCAL_LOG("Destroying");
      Shutdown();
      LOCAL_LOG("Destroyed");
    }
    catch (const std::exception& ex)
    {
      FSLLOG_ERROR("EGLDemoHost destructor can not throw so aborting. " << ex.what())
      std::abort();
    }
  }


  void EGLDemoHost::OnConstructed()
  {
    // Prepare everything
    Init();
  }


  void EGLDemoHost::OnActivate()
  {
    m_isActivated = true;
  }


  void EGLDemoHost::OnDeactivate()
  {
    m_isActivated = false;
  }


  void EGLDemoHost::OnSuspend()
  {
    Shutdown();
  }


  void EGLDemoHost::OnResume()
  {
    Init();
  }


  DemoHostFeature EGLDemoHost::GetActiveAPI() const
  {
    return m_activeApi;
  }


  Point2 EGLDemoHost::GetScreenResolution() const
  {
    assert(!m_enableGLES || m_eglContextClientVersionMajor > 0);
    // The EGL state should be 100% ready
    assert(m_hDisplay != EGL_NO_DISPLAY);
    assert(m_hContext != EGL_NO_CONTEXT);
    assert(m_hSurface != EGL_NO_SURFACE);
    assert(m_hConfig != EMPTY_VALUE_EGLCONFIG);
    assert(m_window);

    return GetEGLSurfaceResolution(m_hDisplay, m_hSurface);
  }


  bool EGLDemoHost::SwapBuffers()
  {
    if (!m_isActivated)
      return true;

    // Validate that we are not suspended
    assert(m_window);
    assert(m_hDisplay != EGL_NO_DISPLAY);
    assert(m_hContext != EGL_NO_CONTEXT);
    assert(m_hSurface != EGL_NO_SURFACE);
    assert(m_hConfig != EMPTY_VALUE_EGLCONFIG);
    return eglSwapBuffers(m_hDisplay, m_hSurface) == EGL_TRUE;
  }


  bool EGLDemoHost::ProcessNativeMessages(const bool allowBlock)
  {
    // This will be called while suspended
    // Allow the native window to process messages
    NativeWindowProcessMessagesArgs args(allowBlock);
    return m_windowSystem->ProcessMessages(args);
  }


  void EGLDemoHost::Init()
  {
    LOCAL_LOG("Init");
    try
    {
      // Get the basic GL configuration ready
      InitEGL();

      // Create the main surface and context
      InitSurfaceAndContext();

      //! Give extending classes a chance to react
      m_apiInit = true;
      OnAPIInitialized();
    }
    catch (const std::exception&)
    {
      //LOCAL_LOG("Init failed with exception: " << ex.what());
      Shutdown();
      throw;
    }
  }


  void EGLDemoHost::Shutdown()
  {
    LOCAL_LOG("Shutdown");

    // Give extending classes a chance to react
    if (m_apiInit)
    {
      m_apiInit = false;
      OnAPIShutdown();
    }

    ShutdownSurfaceAndContext();
    ShutdownEGL();
  }


  void EGLDemoHost::InitEGL()
  {
    assert(m_nativeWindowSetup);
    assert(!m_window);
    assert(m_hDisplay == EGL_NO_DISPLAY);
    assert(m_hContext == EGL_NO_CONTEXT);
    assert(m_hSurface == EGL_NO_SURFACE);
    assert(m_hConfig == EMPTY_VALUE_EGLCONFIG);

    // Acquire the various native display handles
    EGLNativeDisplayType hDisplay = m_windowSystem->GetDisplayType();
    try
    {
      m_hDisplay = EGL_CHECK(eglGetDisplay(hDisplay));

      LOCAL_LOG("Initialize");

      // Configure the display
      EGL_CHECK(eglInitialize(m_hDisplay, nullptr, nullptr));

      if (m_enableVG)
      {
        LOCAL_LOG("Binding VG");
        EGL_CHECK(eglBindAPI(EGL_OPENVG_API));
      }
      if (m_enableGLES)
      {
        LOCAL_LOG("Binding GLES");
        EGL_CHECK(eglBindAPI(EGL_OPENGL_ES_API));
      }

      LOCAL_LOG("Choosing config");

      EGLint numConfigs;
      EGL_CHECK(eglChooseConfig(m_hDisplay, &m_finalConfigAttribs[0], &m_hConfig, 1, &numConfigs));

      if (numConfigs != 1)
        InitEGLTryConfigFallback(m_configControl, m_appAglConfigAttribs);

      if (m_logSelectedConfig)
        DoLogConfigComparison(m_hDisplay, m_hConfig, m_finalConfigAttribs);
      if (m_logExtensions)
        DoLogExtensions(m_hDisplay);

      // Do EGL extension check
      DoCheckExtensions(m_hDisplay, m_extensionRequests);

      LOCAL_LOG("Creating native window");

      // Prepare the native window
      const NativeEGLSetup nativeEglSetup(hDisplay, m_hDisplay, m_hConfig);
      m_window = m_windowSystem->CreateNativeWindow(*m_nativeWindowSetup, nativeEglSetup);
      m_windowHostInfoControl->AddWindow(m_window);
    }
    catch (const std::exception&)
    {
      ShutdownEGL();
      throw;
    }
  }

  bool EGLDemoHost::InitEGLTryConfigFallback(const ConfigControl configControl, const std::deque<EGLint>& appAglConfigAttribs)
  {
    if (!m_enableGLES)
      return false;

    FSLLOG("Preferred configuration not available, trying to locate a alternative.");
    if (configControl != ConfigControl::Exact)
    {
      {
        FSLLOG("- Trying with a different depth buffer size.");
        BuildEGLConfig(m_finalConfigAttribs, appAglConfigAttribs, m_configControl, m_featureConfig, m_options, DEFAULT_RGB_CONFIG, FALLBACK_DEPTH_BUFFER_SIZE);

        // Lets try again
        EGLint numConfigs;
        EGL_CHECK(eglChooseConfig(m_hDisplay, &m_finalConfigAttribs[0], &m_hConfig, 1, &numConfigs));
        if (numConfigs == 1)
          return true;
      }

      const RGBConfig configRGB565(5,6,5);
      {
        FSLLOG("- Trying with a different color depth.");
        BuildEGLConfig(m_finalConfigAttribs, appAglConfigAttribs, m_configControl, m_featureConfig, m_options, configRGB565, DEFAULT_DEPTH_BUFFER_SIZE);

        // Lets try again
        EGLint numConfigs;
        EGL_CHECK(eglChooseConfig(m_hDisplay, &m_finalConfigAttribs[0], &m_hConfig, 1, &numConfigs));
        if (numConfigs == 1)
          return true;
      }

      {
        FSLLOG("- Trying with a different color depth and depth buffer size.");

        BuildEGLConfig(m_finalConfigAttribs, appAglConfigAttribs, m_configControl, m_featureConfig, m_options, configRGB565, FALLBACK_DEPTH_BUFFER_SIZE);

        // Lets try again
        EGLint numConfigs;
        EGL_CHECK(eglChooseConfig(m_hDisplay, &m_finalConfigAttribs[0], &m_hConfig, 1, &numConfigs));
        if (numConfigs == 1)
          return true;
      }
    }

    // If the app requested a exact config, lets try to do a merge instead
    if (configControl == ConfigControl::Exact)
    {
      FSLLOG("- Switching to merge app and default config.");
      if (InitEGLTryConfigFallback(ConfigControl::Overwrite, appAglConfigAttribs))
        return true;
    }

    // As a last resort lets try to ignore the app
    if (appAglConfigAttribs.size() > 0)
    {
      FSLLOG("- Ignoring the app requested config.");
      std::deque<EGLint> emptyAppAglConfigAttribs;
      if (InitEGLTryConfigFallback(configControl, appAglConfigAttribs))
        return true;
    }
    return false;
  }


  void EGLDemoHost::InitSurfaceAndContext()
  {
    //! The app manager should not exist when this is called

    assert(!m_enableGLES || m_eglContextClientVersionMajor > 0);
    assert(m_hDisplay != EGL_NO_DISPLAY);
    assert(m_hConfig != EMPTY_VALUE_EGLCONFIG);
    assert(m_window);
    if (m_hDisplay == EGL_NO_DISPLAY || m_hConfig == EMPTY_VALUE_EGLCONFIG || !m_window)
      return;

    try
    {
      const EGLNativeWindowType hWindow = m_window->GetWindowType();

      // Setup the rest
      LOCAL_LOG("Creating window surface");
      m_hSurface = EGL_CHECK(eglCreateWindowSurface(m_hDisplay, m_hConfig, hWindow, nullptr));

      LOCAL_LOG("Creating context");
      m_hContext = DoEGLCreateContext(m_hDisplay, m_hConfig, m_enableGLES, m_eglContextClientVersionMajor, m_eglContextClientVersionMinor, m_eglContextClientVersionMinimumMinor);

      LOCAL_LOG("Making current");
      EGL_CHECK(eglMakeCurrent(m_hDisplay, m_hSurface, m_hSurface, m_hContext));
    }
    catch (const std::exception&)
    {
      ShutdownSurfaceAndContext();
      throw;
    }
  }


  void EGLDemoHost::ShutdownSurfaceAndContext()
  {
    if (m_hDisplay == EGL_NO_DISPLAY)
      return;

    assert(m_hConfig != EMPTY_VALUE_EGLCONFIG);
    assert(m_window);

    EGL_LOG_ERROR(eglMakeCurrent(m_hDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));

    assert(eglGetError() == EGL_SUCCESS);
    if (m_hContext != EGL_NO_CONTEXT)
    {
      LOCAL_LOG("Destroying context");
      m_hostService->SetContext(EGL_NO_CONTEXT);
      // NOTE: ARM-EMU - For some reason this gives a error on shutdown
      EGL_LOG_ERROR(eglDestroyContext(m_hDisplay, m_hContext));
      m_hContext = EGL_NO_CONTEXT;
    }

    if (m_hSurface != EGL_NO_SURFACE)
    {
      LOCAL_LOG("Destroying surface");
      m_hostService->SetSurface(EGL_NO_SURFACE);
      EGL_LOG_ERROR(eglDestroySurface(m_hDisplay, m_hSurface));
      m_hSurface = EGL_NO_SURFACE;
    }
  }


  void EGLDemoHost::ShutdownEGL()
  {
    //! The app manager should have been released before calling this
    assert(m_hContext == EGL_NO_CONTEXT);
    assert(m_hSurface == EGL_NO_SURFACE);

    if (m_hDisplay != EGL_NO_DISPLAY)
    {
      LOCAL_LOG("Destroying window");
      m_hostService->SetDisplay(EGL_NO_DISPLAY);
      // Release the native window
      m_windowHostInfoControl->RemoveWindow(m_window);
      m_window.reset();

      m_hConfig = EMPTY_VALUE_EGLCONFIG;
      LOCAL_LOG("Terminating egl");
      eglTerminate(m_hDisplay);
      m_hDisplay = EGL_NO_DISPLAY;
      eglReleaseThread();
    }
  }
}
