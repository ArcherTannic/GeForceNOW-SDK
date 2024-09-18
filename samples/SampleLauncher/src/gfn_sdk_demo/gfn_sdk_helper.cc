// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2019-2021 NVIDIA Corporation. All rights reserved.

#include "gfn_sdk_demo/gfn_sdk_helper.h"

#include "include/cef_parser.h"
#include "include/cef_command_line.h"
#include "shared/client_util.h"
#include "shared/defines.h"
#include "shared/main.h"
#include "GfnRuntimeSdk_Wrapper.h"  //Helper functions that wrap Library-based APIs
#include <fstream>

#ifdef _WIN32
#   define HELPER_CALLBACK __stdcall
#else
#   define HELPER_CALLBACK
#endif

#include "GfnCloudCheckUtils.h"
#define CLOUD_CHECK_MIN_NONCE_SIZE 16

CefString GFN_SDK_INIT = "GFN_SDK_INIT";
CefString GFN_SDK_SHUTDOWN = "GFN_SDK_SHUTDOWN";
CefString GFN_SDK_STREAM_ACTION = "GFN_SDK_STREAM_ACTION";
CefString GFN_SDK_IS_RUNNING_IN_CLOUD = "GFN_SDK_IS_RUNNING_IN_CLOUD";
CefString GFN_SDK_IS_RUNNING_IN_CLOUD_SECURE = "GFN_SDK_IS_RUNNING_IN_CLOUD_SECURE";
CefString GFN_SDK_CLOUD_CHECK_WITH_VALIDATION = "GFN_SDK_CLOUD_CHECK_WITH_VALIDATION";
CefString GFN_SDK_CLOUD_CHECK_NO_VALIDATION = "GFN_SDK_CLOUD_CHECK_NO_VALIDATION";
CefString GFN_SDK_GET_CLIENT_IP = "GFN_SDK_GET_CLIENT_IP";
CefString GFN_SDK_GET_CLIENT_COUNTRY_CODE = "GFN_SDK_GET_CLIENT_COUNTRY_CODE";
CefString GFN_SDK_GET_CLIENT_LANGUAGE_CODE = "GFN_SDK_GET_CLIENT_LANGUAGE_CODE";
CefString GFN_SDK_REGISTER_STREAM_STATUS_CALLBACK = "GFN_SDK_REGISTER_STREAM_STATUS_CALLBACK";
CefString GFN_SDK_IS_TITLE_AVAILABLE = "GFN_SDK_IS_TITLE_AVAILABLE";
CefString GFN_SDK_GET_AVAILABLE_TITLES = "GFN_SDK_GET_AVAILABLE_TITLES";
CefString GFN_SDK_GET_PARTNER_SECURE_DATA = "GFN_SDK_GET_PARTNER_SECURE_DATA";
CefString GET_TCP_PORT = "GET_TCP_PORT";
CefString GET_CLIENT_INFO = "GFN_SDK_GET_CLIENT_INFO";
CefString GFN_SDK_REGISTER_CLIENT_INFO_CALLBACK = "GFN_SDK_REGISTER_CLIENT_INFO_CALLBACK";
CefString GFN_SDK_REGISTER_NETWORK_STATUS_CALLBACK = "GFN_SDK_REGISTER_NETWORK_STATUS_CALLBACK";
CefString GET_OVERRIDE_URI = "GET_OVERRIDE_URI";
CefString GFN_SDK_GET_PARTNER_DATA = "GFN_SDK_GET_PARTNER_DATA";
CefString GET_SESSION_INFO = "GFN_SDK_GET_SESSION_INFO";
CefString GFN_SDK_SEND_MESSAGE = "GFN_SDK_SEND_MESSAGE";
CefString GFN_SDK_REGISTER_MESSAGE_CALLBACK = "GFN_SDK_REGISTER_MESSAGE_CALLBACK";
CefString GFN_SDK_GET_ADDITIONAL_SUPPORTED_TITLES = "GFN_SDK_GET_ADDITIONAL_SUPPORTED_TITLES";
CefString GFN_SDK_OPEN_URL_ON_CLIENT = "GFN_SDK_OPEN_URL_ON_CLIENT";

static CefString DictToJson(CefRefPtr<CefDictionaryValue> dict)
{
    auto json = CefValue::Create();
    json->SetDictionary(dict);
    return CefWriteJSON(json, JSON_WRITER_DEFAULT);
}

static void logGfnSdkData()
{
#ifdef _WIN32
    std::wstring appDataPath;
    shared::TryGetSpecialFolderPath(shared::SD_COMMONAPPDATA, appDataPath);
    std::wstring dataFilePath = appDataPath + L"\\NVIDIA Corporation\\GfnRuntimeSdk\\GFNSDK_Data.json";
#else
    std::string dataFilePath = "/var/opt/nvidia/GfnRuntimeSdk/GFNSDK_Data.json";
#endif
    std::ifstream ifs(dataFilePath);
    if (!ifs)
    {
        LOG(ERROR) << "Could not locate " << dataFilePath;
    }
    else
    {
        std::string contents;

        ifs.seekg(0, std::ios::end);
        contents.reserve(ifs.tellg());
        ifs.seekg(0, std::ios::beg);

        contents.assign((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

        LOG(INFO) << "Contents of " << dataFilePath << ":";
        LOG(INFO) << contents;
        ifs.close();
    }
}

// Callback function for handling stream status callbacks
static void HELPER_CALLBACK handleStreamStatusCallback(GfnStreamStatus status, void* context);
static void HELPER_CALLBACK handleNetworkStatusCallback(GfnNetworkStatusUpdateData* pNetworkStatus, void* context);
static void HELPER_CALLBACK handleClientInfoCallback(GfnClientInfoUpdateData* pClientUpdate, void* context);
static CefMessageRouterBrowserSide::Callback* s_registerStreamStatusCallback = nullptr;
static CefMessageRouterBrowserSide::Callback* s_registerNetworkStatusCallback = nullptr;
static CefMessageRouterBrowserSide::Callback* s_registerClientInfoCallback = nullptr;

static void HELPER_CALLBACK handleMessageCallback(GfnString* pStrData, void* context);
static CefMessageRouterBrowserSide::Callback* s_registerMessageCallback = nullptr;

static GfnError initGFN()
{
    GfnError err = GfnInitializeSdk(GfnDisplayLanguage::gfnDefaultLanguage);
    switch (err)
    {
    case GfnError::gfnSuccess:
        LOG(INFO) << "succeeded at initializing GFNRuntime SDK";
        logGfnSdkData();
        break;
    case GfnError::gfnInitSuccessClientOnly:
        LOG(INFO) << "succeeded at initializing client-only GFNRuntime SDK";
        break;
    default:
        LOG(ERROR) << "error initializing: " << GfnErrorToString(err);
    }

    return err;
}

bool GfnSdkHelper(CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    int64 query_id,
    const CefString& request,
    bool persistent,
    CefRefPtr<CefMessageRouterBrowserSide::Callback> callback)
{
    CefRefPtr<CefValue> requestValue = CefParseJSON(request, JSON_PARSER_RFC);

    CefRefPtr<CefDictionaryValue> dict = requestValue->GetDictionary();
    CefString command = dict->GetString("command");

    /**
     * Query command for initializing the GFN SDK. Should be called once during launcher startup
     * before making any other GFN SDK calls.
     */
    if (command == GFN_SDK_INIT)
    {
        GfnError err = initGFN();
        CefRefPtr<CefDictionaryValue> response_dict = CefDictionaryValue::Create();
        // gfnInitSuccessClientOnly is also success, and needs to be treated as such
        response_dict->SetBool("success", (err == GfnError::gfnSuccess || err == GfnError::gfnInitSuccessClientOnly));
        response_dict->SetString("errorMessage", GfnErrorToString(err));

        CefString response(DictToJson(response_dict));
        callback->Success(response);
        return true;
    }
    /**
     * Query command for shutting down the GFN SDK. Should be called once the SDK is no longer needed.
     * Manually wire up into gfn_sdk.html where you want shutdown to occur.
     */
    if (command == GFN_SDK_SHUTDOWN)
    {
        GfnError err = GfnShutdownSdk();
        CefRefPtr<CefDictionaryValue> response_dict = CefDictionaryValue::Create();
        response_dict->SetBool("success", (err == GfnError::gfnSuccess));
        response_dict->SetString("errorMessage", GfnErrorToString(err));

        CefString response(DictToJson(response_dict));
        callback->Success(response);
        return true;
    }
    /**
     * Calls into GFN SDK to determine whether the sample launcher is being executed inside
     * an NVIDIA GeForce NOW game seat.
     */
    else if (command == GFN_SDK_IS_RUNNING_IN_CLOUD)
    {
        bool enabled = false;
        GfnError err = GfnIsRunningInCloud(&enabled);
        if (err != GfnError::gfnSuccess)
        {
            LOG(ERROR) << "Failed to get if running in cloud. Error: " << err;
        }

        LOG(INFO) << "Cloud environment : " << enabled;

        CefRefPtr<CefDictionaryValue> response_dict = CefDictionaryValue::Create();
        response_dict->SetBool("enabled", enabled);
        response_dict->SetString("errorMessage", GfnErrorToString(err));

        CefString response(DictToJson(response_dict));
        callback->Success(response);

        return true;
    }
    /**
    * Calls into GFN SDK securely to determine whether the sample launcher is being executed inside
    * an NVIDIA GeForce NOW game seat. Call will only succeed if the process has been registered
    * with NVIDIA, which this Sample Launcher has, but will fail if the process name/code has been
    * altered.
    */
    else if (command == GFN_SDK_IS_RUNNING_IN_CLOUD_SECURE)
    {
        GfnIsRunningInCloudAssurance assurance = GfnIsRunningInCloudAssurance::gfnNotCloud;

        GfnError err = GfnIsRunningInCloudSecure(&assurance);
        if (err != GfnError::gfnSuccess)
        {
            LOG(ERROR) << "Failed to get if running in cloud. Error: " << GfnErrorToString(err);
        }

        LOG(INFO) << "Cloud environment assurance value: " << assurance;

        CefRefPtr<CefDictionaryValue> response_dict = CefDictionaryValue::Create();
        response_dict->SetInt("assurance", assurance);
        response_dict->SetString("errorMessage", GfnErrorToString(err));

        CefString response(DictToJson(response_dict));
        callback->Success(response);

        return true;
    }
    /**
    * Calls into GFN SDK securely to determine whether the sample launcher is being executed inside
    * a NVIDIA GeForce NOW game seat. This version passes a nonce in challenge and validates the response
    */
    else if (command == GFN_SDK_CLOUD_CHECK_WITH_VALIDATION)
    {
        CefRefPtr<CefDictionaryValue> response_dict = CefDictionaryValue::Create();
        bool bCloudCheck = false;
        char nonce[CLOUD_CHECK_MIN_NONCE_SIZE] = { 0 };
        if (!GfnCloudCheckGenerateNonce(nonce, CLOUD_CHECK_MIN_NONCE_SIZE))
        {
            LOG(ERROR) << "Generate nonce failed";
            response_dict->SetString("errorMessage", CefString("Generate nonce failed"));
        }
        else
        {
            GfnCloudCheckChallenge challenge = { nonce, CLOUD_CHECK_MIN_NONCE_SIZE };
            GfnCloudCheckResponse response = { NULL, 0 };

            GfnError err = GfnCloudCheck(&challenge, &response, &bCloudCheck);
            if (err != GfnError::gfnSuccess)
            {
                LOG(ERROR) << "Cloud check API error: " << GfnErrorToString(err);
            }
            else
            {
                if (response.attestationData != NULL)
                {
                    if (GfnCloudCheckVerifyAttestationData(response.attestationData, challenge.nonce, challenge.nonceSize))
                    {
                        LOG(INFO) << "CloudCheck response validated";
                    }
                    else
                    {
                        LOG(INFO) << "CloudCheck response validation failed";
                    }
                    GfnFree(&response.attestationData);
                }
            }
            response_dict->SetBool("isCloudEnvironment", bCloudCheck);
            response_dict->SetString("errorMessage", GfnErrorToString(err));
        }

        LOG(INFO) << "Cloud environment : " << bCloudCheck;
        CefString resp(DictToJson(response_dict));
        callback->Success(resp);
        return true;
    }
    /**
    * Calls into GFN SDK securely to determine whether the sample launcher is being executed inside
    * a NVIDIA GeForce NOW game seat. This version passes null value for challenge and response and gets
    * the boolean value that indicates cloud environment.
    **/
    else if (command == GFN_SDK_CLOUD_CHECK_NO_VALIDATION)
    {
        CefRefPtr<CefDictionaryValue> response_dict = CefDictionaryValue::Create();
        bool bCloudCheckSimple = false;

        GfnError err = GfnCloudCheck(nullptr, nullptr, &bCloudCheckSimple);
        if (err != GfnError::gfnSuccess)
        {
            LOG(ERROR) << "Cloud check API (simple) error: " << GfnErrorToString(err);
        }
        response_dict->SetBool("isCloudEnvironment", bCloudCheckSimple);
        response_dict->SetString("errorMessage", GfnErrorToString(err));

        LOG(INFO) << "Cloud environment : " << bCloudCheckSimple;
        CefString resp(DictToJson(response_dict));
        callback->Success(resp);
        return true;
    }
    /**
     * Calls into GFN SDK to determine if a specific title is available to stream right now
     * directly from the NVIDIA GeForce NOW game seat.
     */
    else if (command == GFN_SDK_IS_TITLE_AVAILABLE)
    {
        CefRefPtr<CefDictionaryValue> response_dict = CefDictionaryValue::Create();
        if (dict->HasKey("appId"))
        {
            std::string pchappId = dict->GetString("appId").ToString();
            bool available = false;
            GfnError err = GfnIsTitleAvailable(pchappId.c_str(), &available);
            if (err != GfnError::gfnSuccess)
            {
                LOG(ERROR) << "is title available error: " << GfnErrorToString(err);
            }
            else
            {
                LOG(INFO) << "is available to stream: " << available;
            }

            response_dict = CefDictionaryValue::Create();
            response_dict->SetBool("available", available);
        }
        else
        {
            response_dict->SetString("errorMessage", CefString("Bad arguments to CEF extension"));
        }

        CefString response(DictToJson(response_dict));
        callback->Success(response);
        return true;
    }
    /**
     * Calls into GFN SDK to determine all titles available to stream right now directly from the
     * NVIDIA GeForce NOW game seat.
     */
    else if (command == GFN_SDK_GET_AVAILABLE_TITLES)
    {
        CefRefPtr<CefDictionaryValue> response_dict = CefDictionaryValue::Create();
        char const* appIds = nullptr;
        GfnError err = GfnGetTitlesAvailable(&appIds);
        if (err != GfnError::gfnSuccess)
        {
            LOG(ERROR) << "get available titles error: " << GfnErrorToString(err);
            response_dict->SetString("titles", "");
        }
        else
        {
            response_dict->SetString("titles", appIds);
            GfnFree(&appIds);
        }

        response_dict->SetString("errorMessage", GfnErrorToString(err));
        CefString response(DictToJson(response_dict));
        callback->Success(response);

        return true;
    }
    /**
     * Calls into GFN SDK to initiate a new GeForce NOW streaming session and launch the specified
     * game application.
     * If the user is logged in to Geforce NOW streaming client, this call will begin streaming
     * immediately.
     * If the user is not logged in, the Geforce NOW streaming client will display a login
     * window prompting the user to authenticate first.
     */
    else if (command == GFN_SDK_STREAM_ACTION)
    {
        bool actionSuccess = false;
        bool launchStream = false;
        std::string msg;
        if (!dict->HasKey("launchStream"))
        {
            msg = "Call to Stream action missing \"launchStream\" parameter";
        }
        launchStream = dict->GetBool("launchStream");
        if (launchStream)
        {
            LOG(INFO) << "Received request to start a session";
            if (dict->HasKey("gfnTitleId"))
            {
                StartStreamResponse response = { 0 };
                StartStreamInput startStreamInput = { 0 };
                uint32_t gfnTitleId = dict->GetInt("gfnTitleId");
                startStreamInput.uiTitleId = gfnTitleId;

                // 3rd party IDM token for SSO that is consumed by launcher application running in GFN
                std::string pchcustAuth = dict->GetString("launcherToken").ToString();
                startStreamInput.pchPartnerSecureData = pchcustAuth.c_str();

                startStreamInput.pchPartnerData = "This is example custom data";

                GfnError err = GfnStartStream(&startStreamInput, &response);
                msg = "gfnStartStream = " + std::string(GfnErrorToString(err));
                if (err != GfnError::gfnSuccess)
                {
                    LOG(ERROR) << "launch game error: " << msg;
                }
                else
                {
                    actionSuccess = true;
                    msg = msg + ", GFN Downloaded & Installed = " + (response.downloaded ? "Yes" : "Not needed");
                    LOG(INFO) << "launch game response. Downloaded GeForceNOW? : " << response.downloaded;
                }
            }
            else
            {
                msg = "Bad arguments to CEF extension";
            }
        }
        else
        {
            LOG(INFO) << "Received request to stop a session";
            GfnError err = GfnStopStream();
            msg = "gfnStopStream = " + std::string(GfnErrorToString(err));
            if (err != GfnError::gfnSuccess)
            {
                LOG(ERROR) << "Stream stop error: " << msg;
            }
            else
            {
                actionSuccess = true;
                LOG(INFO) << "Stream stop success";
            }
        }

        CefRefPtr<CefDictionaryValue> response_dict = CefDictionaryValue::Create();
        response_dict->SetBool("actionSuccess", actionSuccess);
        response_dict->SetString("errorMessage", CefString(msg.c_str()));

        CefString response(DictToJson(response_dict));
        callback->Success(response);
        return true;
    }
    else if (command == GFN_SDK_SEND_MESSAGE)
    {
        bool actionSuccess = false;
        std::string statusMsg;
        if (!dict->HasKey("launchStream"))
        {
            statusMsg = "Call to SendMessage missing \"message\" parameter";
        }

        std::string message = dict->GetString("message").ToString();

        GfnError err = GfnSendMessage(message.c_str(), (unsigned int)message.length());
        statusMsg = "GfnSendMessage = " + std::string(GfnErrorToString(err));
        if (err != GfnError::gfnSuccess)
        {
            LOG(ERROR) << "SendMessage error: " << statusMsg;
        }
        else
        {
            actionSuccess = true;
            LOG(INFO) << "SendMessage success";
        }

        CefRefPtr<CefDictionaryValue> response_dict = CefDictionaryValue::Create();
        response_dict->SetBool("actionSuccess", actionSuccess);
        response_dict->SetString("errorMessage", CefString(statusMsg.c_str()));

        CefString response(DictToJson(response_dict));
        callback->Success(response);
        return true;
    }
    else if (command == GFN_SDK_REGISTER_MESSAGE_CALLBACK)
    {
        s_registerMessageCallback = callback;

        GfnError err = GfnRegisterMessageCallback(reinterpret_cast<MessageCallbackSig>(&handleMessageCallback), nullptr);
        if (err != GfnError::gfnSuccess)
        {
            LOG(ERROR) << "Failed to register Message Callback: " << GfnErrorToString(err);
        }
        return true;
    }
    /**
     * Calls into GFN SDK to get the user's current local IP address. This is meant to be
     * called while running on the GeForce NOW game seat to handle scenarios where the IP
     * addressed is used for account security or other validation purposes or things like
     * region specific localization or other UI.
     */
    else if (command == GFN_SDK_GET_CLIENT_IP)
    {
        char const* clientIp = nullptr;
        GfnError err = GfnGetClientIpV4(&clientIp);
        if (err != GfnError::gfnSuccess)
        {
            LOG(ERROR) << "get client IP error: " << GfnErrorToString(err);
        }
        else
        {
            LOG(INFO) << "client ip: " << clientIp;
        }

        CefRefPtr<CefDictionaryValue> response_dict = CefDictionaryValue::Create();
        response_dict->SetString("clientIp", clientIp);
        response_dict->SetString("errorMessage", GfnErrorToString(err));

        CefString response(DictToJson(response_dict));
        callback->Success(response);

        return true;
    }
    /**
     * Calls into GFN SDK to get the user's country code. This is meant to be
     * called while running on the GeForce NOW game seat to determine the country where
     * user is located
     */
    else if (command == GFN_SDK_GET_CLIENT_COUNTRY_CODE)
    {
        char clientCountryCode[3] = { 0 };
        GfnError err = GfnGetClientCountryCode(clientCountryCode, 3);
        if (err != GfnError::gfnSuccess)
        {
            LOG(ERROR) << "get client country code error: " << GfnErrorToString(err);
        }
        else
        {
            LOG(INFO) << "client country code: " << clientCountryCode;
        }

        CefRefPtr<CefDictionaryValue> response_dict = CefDictionaryValue::Create();
        response_dict->SetString("clientCountryCode", clientCountryCode);
        response_dict->SetString("errorMessage", GfnErrorToString(err));

        CefString response(DictToJson(response_dict));
        callback->Success(response);
        return true;
    }

    /**
     * Calls into GFN SDK to get the user's language code. THis is meant to be
     * called while running on the GeForce NOW game seat to determine the language
     * used by the user.
     */
    else if (command == GFN_SDK_GET_CLIENT_LANGUAGE_CODE)
    {
        const char* clientLanguageCode;
        GfnError err = GfnGetClientLanguageCode(&clientLanguageCode);
        CefRefPtr<CefDictionaryValue> response_dict = CefDictionaryValue::Create();
        if (err != GfnError::gfnSuccess)
        {
            LOG(ERROR) << "get client country code error: " << GfnErrorToString(err);
            response_dict->SetString("clientLanguageCode", "");
            response_dict->SetString("errorMessage", GfnErrorToString(err));
        }
        else
        {
            LOG(INFO) << "client country code: " << clientLanguageCode;
            response_dict->SetString("clientLanguageCode", clientLanguageCode);
            response_dict->SetString("errorMessage", "");
        }

        CefString response(DictToJson(response_dict));
        callback->Success(response);

        if (err == GfnError::gfnSuccess)
        {
            GfnFree(&clientLanguageCode);
        }
        return true;
    }
    /**
     * Registers for callback notifications during a streaming session.
     */
    else if (command == GFN_SDK_REGISTER_STREAM_STATUS_CALLBACK)
    {
        s_registerStreamStatusCallback = callback;

        GfnError err = GfnRegisterStreamStatusCallback(reinterpret_cast<StreamStatusCallbackSig>(&handleStreamStatusCallback), nullptr);
        if (err != GfnError::gfnSuccess)
        {
            LOG(ERROR) << "Failed to register Stream Status Callback: " << GfnErrorToString(err);
        }
        return true;
    }
    /**
     * Requests a copy of the token data that was passed into partnerSecureData as part of the call to one
     * of the StartStream APIs. This call should only be made in the GFN cloud environment.
     */
    else if (command == GFN_SDK_GET_PARTNER_SECURE_DATA)
    {
        char const* partnerSecureData = nullptr;
        CefRefPtr<CefDictionaryValue> response_dict = CefDictionaryValue::Create();
        GfnError err = GfnGetPartnerSecureData(&partnerSecureData);
        response_dict->SetString("errorMessage", GfnErrorToString(err));
        if (err != GfnError::gfnSuccess)
        {
            LOG(ERROR) << "get partner secure data error: " << GfnErrorToString(err);
            response_dict->SetString("partnerSecureData", "");
        }
        else
        {
            response_dict->SetString("partnerSecureData", partnerSecureData);
            GfnFree(&partnerSecureData);
        }

        CefString response(DictToJson(response_dict));
        callback->Success(response);

        return true;
    }
    /**
     * Requests a copy of the custom data that was passed into pchPartnerData as part of the
     * call to one of the StartStream APIs. This call should only be made in the GFN cloud
     * environment.
     */
    else if (command == GFN_SDK_GET_PARTNER_DATA)
    {
        char const* partnerData = nullptr;
        CefRefPtr<CefDictionaryValue> response_dict = CefDictionaryValue::Create();
        GfnError err = GfnGetPartnerData(&partnerData);
        response_dict->SetString("errorMessage", GfnErrorToString(err));
        if (err != GfnError::gfnSuccess)
        {
            LOG(ERROR) << "get partner data error: " << GfnErrorToString(err);
            response_dict->SetString("partnerData", "");
        }
        else
        {
            response_dict->SetString("partnerData", partnerData);
            GfnFree(&partnerData);
        }

        CefString response(DictToJson(response_dict));
        callback->Success(response);

        return true;
    }
    else if (command == GET_TCP_PORT)
    {
        CefRefPtr<CefDictionaryValue> response_dict = CefDictionaryValue::Create();
        response_dict->SetString("port", shared::g_activePort);

        CefString response(DictToJson(response_dict));
        callback->Success(response);

        return true;
    }
    else if (command == GET_CLIENT_INFO)
    {
        LOG(INFO) << "Calling GfnGetClientInfo...";
        CefRefPtr<CefDictionaryValue> response_dict = CefDictionaryValue::Create();
        GfnClientInfo clientInfo = { 0 };
        GfnError error = GfnError::gfnSuccess;
        error = GfnGetClientInfo(&clientInfo);
        LOG(INFO) << "GfnGetClientInfo error result: " << error;
        response_dict->SetString("errorMessage", GfnErrorToString(error));
        if (error != GfnError::gfnSuccess)
        {
            LOG(ERROR) << "get client info data error: " << GfnErrorToString(error);
            response_dict->SetString("clientInfo", "");
        }
        else
        {
            response_dict->SetInt("apiVersion", clientInfo.version);
            response_dict->SetString("country", clientInfo.country);
            response_dict->SetString("ipV4", clientInfo.ipV4);
            response_dict->SetString("locale", clientInfo.locale);
            response_dict->SetInt("osType", clientInfo.osType);
            response_dict->SetInt("rtdLatencyMs", clientInfo.RTDAverageLatencyMs);
            response_dict->SetInt("clientResolutionWidth", clientInfo.clientResolution.horizontalPixels);
            response_dict->SetInt("clientResolutionHeight", clientInfo.clientResolution.verticalPixels);
        }
        CefString response(DictToJson(response_dict));
        LOG(INFO) << "GfnGetClientInfo data: " << response.ToString();
        callback->Success(response);
        return true;
    }
    /**
     * Registers for callback notifications for on-seat client info updates
     */
    else if (command == GFN_SDK_REGISTER_CLIENT_INFO_CALLBACK)
    {
        s_registerClientInfoCallback = callback;

        GfnError err = GfnRegisterClientInfoCallback(reinterpret_cast<ClientInfoCallbackSig>(&handleClientInfoCallback), nullptr);
        if (err != GfnError::gfnSuccess)
        {
            LOG(ERROR) << "Failed to register Client Info Callback: " << GfnErrorToString(err);
        }
        return true;
    }
    /**
     * Registers for callback notifications for on-seat network latency updates
     */
    else if (command == GFN_SDK_REGISTER_NETWORK_STATUS_CALLBACK)
    {
        s_registerNetworkStatusCallback = callback;

        GfnError err = GfnRegisterNetworkStatusCallback(reinterpret_cast<NetworkStatusCallbackSig>(&handleNetworkStatusCallback), 5 * 1000, nullptr);
        if (err != GfnError::gfnSuccess)
        {
            LOG(ERROR) << "Failed to register Network Latency Callback: " << GfnErrorToString(err);
        }
        return true;
    }
    else if (command == GET_OVERRIDE_URI)
    {
        CefString override_uri = "";
        CefRefPtr<CefCommandLine> command_line = CefCommandLine::GetGlobalCommandLine();

        if (command_line->HasSwitch("override_uri"))
        {
            override_uri = command_line->GetSwitchValue("override_uri");
        }

        CefRefPtr<CefDictionaryValue> response_dict = CefDictionaryValue::Create();
        response_dict->SetString("overrideURI", override_uri);

        CefString response(DictToJson(response_dict));
        LOG(INFO) << "Override URI: " << response.ToString();
        callback->Success(response);
        return true;
    }
    else if (command == GET_SESSION_INFO)
    {
        LOG(INFO) << "Calling GfnGetSessionInfo...";
        CefRefPtr<CefDictionaryValue> response_dict = CefDictionaryValue::Create();
        GfnSessionInfo sessionInfo = { 0 };
        GfnError error = GfnError::gfnSuccess;
        error = GfnGetSessionInfo(&sessionInfo);
        LOG(INFO) << "GfnGetSessionInfo error result: " << error;
        response_dict->SetString("errorMessage", GfnErrorToString(error));
        if (error != GfnError::gfnSuccess)
        {
            LOG(ERROR) << "get session info data error: " << GfnErrorToString(error);
            response_dict->SetString("sessionInfo", "");
        }
        else
        {
            response_dict->SetInt("sessionMaxDurationSec", sessionInfo.sessionMaxDurationSec);
            response_dict->SetInt("sessionTimeRemainingSec", sessionInfo.sessionTimeRemainingSec);
            response_dict->SetString("sessionID", sessionInfo.sessionId);
            response_dict->SetInt("sessionRTXEnabled", sessionInfo.sessionRTXEnabled);
        }
        CefString response(DictToJson(response_dict));
        LOG(INFO) << "GfnGetSessionInfo data: " << response.ToString();
        callback->Success(response);
        return true;
    }
    else if (command == GFN_SDK_GET_ADDITIONAL_SUPPORTED_TITLES)
    {
        CefRefPtr<CefListValue> titles_list = CefListValue::Create();

        // Custom title CMS ids could be added here to titles_list

        CefRefPtr<CefDictionaryValue> response_dict = CefDictionaryValue::Create();
        response_dict->SetList("titles", titles_list);

        CefString response(DictToJson(response_dict));
        callback->Success(response);

        return true;
    }
    else if (command == GFN_SDK_OPEN_URL_ON_CLIENT)
    {
        CefRefPtr<CefDictionaryValue> response_dict = CefDictionaryValue::Create();
        GfnError error = GfnOpenURLOnClient("https://www.nvidia.com/en-us/geforce-now/");
        LOG(INFO) << "GfnOpenURLOnClient error result: " << error;
        response_dict->SetString("status", GfnErrorToString(error));
        if (error != GfnError::gfnSuccess)
        {
            LOG(ERROR) << "Open URL on Client data error: " << GfnErrorToString(error);
        }
        CefString response(DictToJson(response_dict));
        callback->Success(response);
        return true;
    }
    LOG(ERROR) << "Unknown command value: " << command;

    return false;
}

void HELPER_CALLBACK handleStreamStatusCallback(GfnStreamStatus status, void* context)
{
    if (s_registerStreamStatusCallback)
    {
        CefRefPtr<CefDictionaryValue> response_dict = CefDictionaryValue::Create();
        response_dict->SetString("status", GfnStreamStatusToString(status));

        CefString response(DictToJson(response_dict));
        s_registerStreamStatusCallback->Success(response);
    }
}

void HELPER_CALLBACK handleNetworkStatusCallback(GfnNetworkStatusUpdateData* pNetworkStatus, void* context)
{
    if (!pNetworkStatus)
    {
        return;
    }
    if (s_registerNetworkStatusCallback && pNetworkStatus->updateType == gfnRTDAverageLatency)
    {
        CefRefPtr<CefDictionaryValue> response_dict = CefDictionaryValue::Create();
        response_dict->SetInt("rtd", pNetworkStatus->data.RTDAverageLatencyMs);

        CefString response(DictToJson(response_dict));
        s_registerNetworkStatusCallback->Success(response);
    }
}
void HELPER_CALLBACK handleClientInfoCallback(GfnClientInfoUpdateData* pClientUpdate, void* context)
{
    if (!pClientUpdate)
    {
        return;
    }
    if (s_registerClientInfoCallback)
    {
        CefRefPtr<CefDictionaryValue> response_dict = CefDictionaryValue::Create();
        switch (pClientUpdate->updateType)
        {
        case gfnOs:
            response_dict->SetInt("os", pClientUpdate->data.osType);
            break;
        case gfnIP:
            response_dict->SetString("IP Changed", pClientUpdate->data.ipV4);
            break;
        case gfnClientResolution:
            response_dict->SetInt("clientResolutionWidth", pClientUpdate->data.clientResolution.horizontalPixels);
            response_dict->SetInt("clientResolutionHeight", pClientUpdate->data.clientResolution.verticalPixels);
            break;
        case gfnSafeZone:
            response_dict->SetDouble("SafeZone.left", pClientUpdate->data.safeZone.value1);
            response_dict->SetDouble("SafeZone.top", pClientUpdate->data.safeZone.value2);
            response_dict->SetDouble("SafeZone.right", pClientUpdate->data.safeZone.value3);
            response_dict->SetDouble("SafeZone.bottom", pClientUpdate->data.safeZone.value4);
            response_dict->SetBool("SafeZone.normalized", pClientUpdate->data.safeZone.normalized);
            break;
        default:
            return;
        }

        CefString response(DictToJson(response_dict));
        s_registerClientInfoCallback->Success(response);
    }
}

void HELPER_CALLBACK handleMessageCallback(GfnString* pStrData, void* context)
{
    if (s_registerMessageCallback)
    {
        CefRefPtr<CefDictionaryValue> response_dict = CefDictionaryValue::Create();

        response_dict->SetString("message", std::string(pStrData->pchString, pStrData->length));

        CefString response(DictToJson(response_dict));
        s_registerMessageCallback->Success(response);
    }
}
