#include "StandaloneUIBridge.h"

#include "KiqMainView.h"
#include "vstgui/lib/cframe.h"
#include "vstgui/standalone/include/helpers/appdelegate.h"
#include "vstgui/standalone/include/helpers/windowlistener.h"
#include "vstgui/standalone/include/iapplication.h"

#include <memory>

namespace KickDrum::Standalone {

using namespace VSTGUI;
using namespace VSTGUI::Standalone;

class KiqApplication final : public Application::DelegateAdapter,
                             public WindowListenerAdapter {
public:
    KiqApplication()
        : Application::DelegateAdapter({"Kiq", "1.0.0", VSTGUI_STANDALONE_APP_URI}) {
    }

    void finishLaunching() override {
        bridge_ = std::make_unique<StandaloneUIBridge>();
        if (!bridge_->initialize()) {
            IApplication::instance().quit();
            return;
        }

        WindowConfiguration config;
        config.title = "Kiq — Kick Designer";
        // VSTGUI's macOS standalone backend treats this as the outer window
        // size. Account for the native title bar so the 700-point canvas is
        // not vertically cropped.
        config.size = CPoint(UI::KiqMainView::kDesignWidth,
                             UI::KiqMainView::kDesignHeight + 29.0);
        config.style.border().close().centered();

        window_ = IApplication::instance().createWindow(config, nullptr);
        if (!window_) {
            bridge_->shutdown();
            IApplication::instance().quit();
            return;
        }

        const CRect bounds(0.0, 0.0,
                           UI::KiqMainView::kDesignWidth,
                           UI::KiqMainView::kDesignHeight);
        auto frame = makeOwned<CFrame>(bounds, nullptr);
        frame->addView(new UI::KiqMainView(bounds, *bridge_));
        window_->setContentView(frame);
        window_->registerWindowListener(this);
        window_->show();
        window_->activate();
    }

    void onClosed(const IWindow&) override {
        if (bridge_) {
            bridge_->shutdown();
        }
        window_ = nullptr;
        IApplication::instance().quit();
    }

    void onQuit() override {
        if (bridge_) {
            bridge_->shutdown();
        }
        bridge_.reset();
    }

private:
    std::unique_ptr<StandaloneUIBridge> bridge_;
    WindowPtr window_;
};

} // namespace KickDrum::Standalone

static VSTGUI::Standalone::Application::Init gKiqApplication(
    std::make_unique<KickDrum::Standalone::KiqApplication>());
