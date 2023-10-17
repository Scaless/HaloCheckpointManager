#pragma once
#include "GameState.h"
#include "IGUIElement.h"
#include "IAnchorPoint.h"
#include "GUIElementStore.h"
#include "IMCCStateHook.h"
#include "Vec2.h"
#include "HotkeyRenderer.h"
#include "GUIMCCState.h"
#include "ControlServiceContainer.h"
#include "SettingsStateAndEvents.h"

class HCMInternalGUI : public IAnchorPoint, public std::enable_shared_from_this<HCMInternalGUI>
{
private:
	std::mutex mDestructionGuard;
	std::mutex currentGameGUIElementsMutex;

	// callbacks
	ScopedCallback<RenderEvent> mImGuiRenderCallbackHandle;
	ScopedCallback< eventpp::CallbackList<void(const MCCState&)>> mMCCStateChangedCallbackHandle;

	void onGUIShowingFreesMCCCursorChanged(bool& newval);
	ScopedCallback<ToggleEvent> mGUIShowingFreesMCCCursorCallbackHandle;

	//injected services
	gsl::not_null<std::shared_ptr<IMCCStateHook>> mccStateHook;
	gsl::not_null<std::shared_ptr<GUIElementStore>> mGUIStore;
	gsl::not_null<std::shared_ptr<HotkeyRenderer>> mHotkeyRenderer;
	gsl::not_null<std::shared_ptr<ControlServiceContainer>> mControlServices;
	gsl::not_null<std::shared_ptr<SettingsStateAndEvents>> mSettings;

	// What we run when ImGuiManager ImGuiRenderEvent is invoked
	void onImGuiRenderEvent(Vec2 ss);
	void onGameStateChange(const MCCState&);

	// initialize resources in the first onImGuiRenderEvent
	void initializeHCMInternalGUI();
	bool m_HCMInternalGUIinitialized = false;


	void primaryRender(); // Primary render function

	// sub-render functions to help my sanity
	std::vector<HCMExceptionBase> errorsToDisplay;
	void renderErrorDialog();

	// GUI data
	ImGuiWindowFlags windowFlags;
	bool m_WindowOpen;
	Vec2 mWindowSize{ 500, 500 };
	Vec2 mWindowPos{ 10, 25 };

	Vec2 mFullScreenSize;
	GUIMCCState mGUIMCCState;

	

	const std::vector<std::shared_ptr<IGUIElement>>* p_currentGameGUIElements = &mGUIStore->getTopLevelGUIElements(GameState::Value::Halo1);



public:

	// Gets passed ImGuiManager ImGuiRenderEvent reference so we can subscribe and unsubscribe
	explicit HCMInternalGUI(std::shared_ptr<IMCCStateHook> MCCStateHook, std::shared_ptr< GUIElementStore> guistore, std::shared_ptr<HotkeyRenderer> hotkeyRenderer, std::shared_ptr<RenderEvent> pRenderEvent, std::shared_ptr<eventpp::CallbackList<void(const MCCState&)>> pMCCStateChangeEvent, std::shared_ptr<ControlServiceContainer> control, std::shared_ptr<SettingsStateAndEvents> settings)
		: mDestructionGuard(),
		currentGameGUIElementsMutex(),
		mImGuiRenderCallbackHandle(pRenderEvent, [this](ImVec2 ss) {onImGuiRenderEvent(ss); }),
		mMCCStateChangedCallbackHandle(pMCCStateChangeEvent, [this](const MCCState& n) { onGameStateChange(n); }),
		mGUIStore(guistore),
		mccStateHook(MCCStateHook), mHotkeyRenderer(hotkeyRenderer), mGUIMCCState(MCCStateHook),
		mControlServices(control),
		mSettings(settings),
		mGUIShowingFreesMCCCursorCallbackHandle(settings->GUIShowingFreesCursor->valueChangedEvent, [this](bool& n) { onGUIShowingFreesMCCCursorChanged(n); })
	{
		PLOG_VERBOSE << "HCMInternalGUI finished construction";
	}

	~HCMInternalGUI()
	{
		// no new callback invocations
		mImGuiRenderCallbackHandle.~ScopedCallback();
		mMCCStateChangedCallbackHandle.~ScopedCallback();
		std::unique_lock<std::mutex> lock(mDestructionGuard); // block until callbacks finish executing
	}


	void addPopupError(HCMExceptionBase error) { if (errorsToDisplay.size() < 5) errorsToDisplay.push_back(error); }
	bool isWindowOpen() { return m_WindowOpen; };
	virtual Vec2 getAnchorPoint() override { return { 10, m_WindowOpen ? (mWindowSize.y + 10) : 30 }; } // anchors bottom left of window
};

// Todo: properly split the concepts of "desired window height" and "allocated window height". maybe call the former one "content height"