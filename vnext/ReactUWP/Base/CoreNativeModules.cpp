// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "pch.h"
#include "CoreNativeModules.h"

// Modules
#include <AsyncStorageModule.h>
#include <Modules/AlertModuleUwp.h>
#include <Modules/Animated/NativeAnimatedModule.h>
#include <Modules/AppStateModuleUwp.h>
#include <Modules/AppThemeModuleUwp.h>
#include <Modules/ClipboardModule.h>
#include <Modules/DeviceInfoModule.h>
#include <Modules/ImageViewManagerModule.h>
#include <Modules/LinkingManagerModule.h>
#include <Modules/LocationObserverModule.h>
#include <Modules/NativeUIManager.h>
#include <Modules/NetworkingModule.h>
#include <Modules/StatusBarModule.h>
#include <Modules/UIManagerModule.h>
#include <Modules/WebSocketModuleUwp.h>
#include <ReactUWP/Modules/I18nModule.h>
#include <Threading/MessageQueueThreadFactory.h>

// ReactWindowsCore
#include <CreateModules.h>

namespace react::uwp {

namespace {

bool HasPackageIdentity() noexcept {
  static const bool hasPackageIdentity = []() noexcept {
    auto packageStatics = winrt::get_activation_factory<winrt::Windows::ApplicationModel::IPackageStatics>(
        winrt::name_of<winrt::Windows::ApplicationModel::Package>());
    auto abiPackageStatics = static_cast<winrt::impl::abi_t<winrt::Windows::ApplicationModel::IPackageStatics> *>(
        winrt::get_abi(packageStatics));
    winrt::com_ptr<winrt::impl::abi_t<winrt::Windows::ApplicationModel::IPackage>> dummy;
    return abiPackageStatics->get_Current(winrt::put_abi(dummy)) !=
        winrt::impl::hresult_from_win32(APPMODEL_ERROR_NO_PACKAGE);
  }
  ();

  return hasPackageIdentity;
}

} // namespace

std::vector<facebook::react::NativeModuleDescription> GetCoreModules(
    std::shared_ptr<facebook::react::IUIManager> uiManager,
    const std::shared_ptr<facebook::react::MessageQueueThread> &messageQueue,
    const std::shared_ptr<facebook::react::MessageQueueThread> &uiMessageQueue,
    std::shared_ptr<DeviceInfo> deviceInfo,
    std::shared_ptr<facebook::react::DevSettings> devSettings,
    const I18nModule::I18nInfo &&i18nInfo,
    std::shared_ptr<facebook::react::AppState> appstate,
    std::shared_ptr<react::windows::AppTheme> appTheme,
    std::weak_ptr<IReactInstance> uwpInstance) noexcept {
  // Modules
  std::vector<facebook::react::NativeModuleDescription> modules;

  modules.emplace_back(
      "UIManager",
      [uiManager = std::move(uiManager), uiMessageQueue]() {
        return facebook::react::createUIManagerModule(std::shared_ptr(uiManager), std::shared_ptr(uiMessageQueue));
      },
      messageQueue);

  modules.emplace_back(
      react::uwp::WebSocketModule::Name,
      []() { return std::make_unique<react::uwp::WebSocketModule>(); },
      MakeSerialQueueThread());

  modules.emplace_back(
      NetworkingModule::Name, []() { return std::make_unique<NetworkingModule>(); }, MakeSerialQueueThread());

  modules.emplace_back(
      "Timing", [messageQueue]() { return facebook::react::CreateTimingModule(messageQueue); }, messageQueue);

  modules.emplace_back(
      DeviceInfoModule::name, [deviceInfo]() { return std::make_unique<DeviceInfoModule>(deviceInfo); }, messageQueue);

  modules.emplace_back(
      LinkingManagerModule::name, []() { return std::make_unique<LinkingManagerModule>(); }, messageQueue);

  modules.emplace_back(
      ImageViewManagerModule::name,
      [messageQueue]() { return std::make_unique<ImageViewManagerModule>(messageQueue); },
      messageQueue);

  modules.emplace_back(
      LocationObserverModule::name,
      [messageQueue]() { return std::make_unique<LocationObserverModule>(messageQueue); },
      MakeSerialQueueThread()); // TODO: figure out threading

  modules.emplace_back(
      facebook::react::AppStateModule::name,
      [appstate = std::move(appstate)]() mutable {
        return std::make_unique<facebook::react::AppStateModule>(std::move(appstate));
      },
      MakeSerialQueueThread());

  modules.emplace_back(
      react::windows::AppThemeModule::name,
      [appTheme = std::move(appTheme)]() mutable {
        return std::make_unique<react::windows::AppThemeModule>(std::move(appTheme));
      },
      messageQueue);

  modules.emplace_back(AlertModule::name, []() { return std::make_unique<AlertModule>(); }, messageQueue);

  modules.emplace_back(ClipboardModule::name, []() { return std::make_unique<ClipboardModule>(); }, messageQueue);

  modules.emplace_back(StatusBarModule::name, []() { return std::make_unique<StatusBarModule>(); }, messageQueue);

  modules.emplace_back(
      NativeAnimatedModule::name,
      [uwpInstance = std::move(uwpInstance)]() mutable {
        return std::make_unique<NativeAnimatedModule>(std::move(uwpInstance));
      },
      messageQueue);

  modules.emplace_back(
      "I18nManager",
      [i18nInfo = std::move(i18nInfo)]() mutable {
        return createI18nModule(std::make_unique<I18nModule>(std::move(i18nInfo)));
      },
      messageQueue);

  // AsyncStorageModule doesn't work without package identity (it indirectly depends on
  // Windows.Storage.StorageFile), so check for package identity before adding it.
  if (HasPackageIdentity()) {
    modules.emplace_back(
        "AsyncLocalStorage",
        []() { return std::make_unique<facebook::react::AsyncStorageModule>(L"asyncStorage"); },
        MakeSerialQueueThread());
  }

  return modules;
}

} // namespace react::uwp
