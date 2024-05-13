#include "pch.h"
#include "ModalDialogRenderer.h"
#include "GlobalKill.h"
#include "WaypointList.h"


class ModalDialogRenderer::ModalDialogRendererImpl
{
private:

		std::shared_ptr<IModalDialog> currentOpenDialog; // null when not rendering a dialog
	

		ScopedCallback<RenderEvent> mImGuiRenderCallbackHandle;
		void onImGuiRenderEvent(SimpleMath::Vector2 screenSize)
		{
			if (currentOpenDialog.get())
			{
				currentOpenDialog->render(screenSize);
			}

#ifdef HCM_DEBUG
			if (GetKeyState('9') & 0x8000)
			{
				PLOG_VERBOSE << "currentOpenDialog: " << ((bool)currentOpenDialog.get() ? "true" : "false");
			}
#endif

		}


		std::optional<std::shared_ptr< FreeMCCCursor>> mFreeMCCCursorService;
		std::optional<std::shared_ptr< BlockGameInput>> mBlockGameInputService;
		std::optional<std::shared_ptr< PauseGame>> mPauseGameService;
		std::shared_ptr<GenericScopedServiceProvider> mHotkeyDisabler;

		std::shared_ptr<GUIServiceInfo> guiFailures;

		std::tuple<std::unique_ptr<ScopedServiceRequest>, std::unique_ptr<ScopedServiceRequest>, std::unique_ptr<ScopedServiceRequest>> getScopedRequests()
		{
			std::unique_ptr<ScopedServiceRequest> scopedFreeCursorRequest;
			if (mFreeMCCCursorService.has_value())
			{
				PLOG_DEBUG << "ModalDialogRenderer requesting freeMCCCursor";
				scopedFreeCursorRequest = mFreeMCCCursorService.value()->scopedRequest(nameof(showCheckpointDumpNameDialog));
			}

			std::unique_ptr<ScopedServiceRequest> scopedBlockInputRequest;
			if (mBlockGameInputService.has_value())
			{
				PLOG_DEBUG << "ModalDialogRenderer requesting blockGameInput";
				scopedBlockInputRequest = mBlockGameInputService.value()->scopedRequest(nameof(showCheckpointDumpNameDialog));
			}

			std::unique_ptr<ScopedServiceRequest> scopedPauseRequest;
			if (mPauseGameService.has_value())
			{
				PLOG_DEBUG << "ModalDialogRenderer requesting pauseGame";
				scopedPauseRequest = mPauseGameService.value()->scopedRequest(nameof(showCheckpointDumpNameDialog));
			}

			return std::tuple<std::unique_ptr<ScopedServiceRequest>, std::unique_ptr<ScopedServiceRequest>, std::unique_ptr<ScopedServiceRequest>>
			{ std::move(scopedFreeCursorRequest) , std::move(scopedBlockInputRequest), std::move(scopedPauseRequest) };
		}

public:

	template <typename ret>
	ret showReturningDialog(std::shared_ptr<IModalDialogReturner<ret>>& dialogToShow)
	{
		PLOG_DEBUG << "showReturningDialog begin";
		if (GlobalKill::isKillSet()) { return dialogToShow->getReturnValue(); } // escape hatch in case a bunch of dialogs are in queue

		// wait for any current rendering dialogs to finish
		while (currentOpenDialog.get())
		{
			Sleep(10);
		}
		PLOG_VERBOSE << "Setting dialog to be rendered";
		currentOpenDialog = dialogToShow; // set to be rendered

		PLOG_DEBUG << "disabling hotkeys and creating scoped requests";
		dialogToShow->disableHotkeys(mHotkeyDisabler); // disable hotkeys while dialog open
		auto scopedRequests = getScopedRequests(); // pause game, free cursor, and block input while dialog open



		while (dialogToShow->isDialogOpen() == true)
		{
			if (GlobalKill::isKillSet()) // uh oh, program shutting down. Let's see if we have time to render one more frame to close the dialog properly.
			{
				dialogToShow->queueEmergencyClose();
				Sleep(30); // give time for one frame to render, hopefully
				currentOpenDialog = nullptr;
				return dialogToShow->getReturnValue();

			}
			else
			{
				Sleep(10); // wait for dialog to close
			}
	
		}

		currentOpenDialog.reset();
		return dialogToShow->getReturnValue();

	}


	void showVoidDialog(std::shared_ptr<IModalDialogVoid>& dialogToShow)
	{
		PLOG_DEBUG << "showVoidDialog begin";
		if (GlobalKill::isKillSet()) { return; } // escape hatch in case a bunch of dialogs are in queue

		// wait for any current rendering dialogs to finish
		while (currentOpenDialog.get())
		{
			Sleep(10);
		}

		PLOG_VERBOSE << "Setting dialog to be rendered";
		currentOpenDialog = dialogToShow; // set to be rendered

		PLOG_DEBUG << "disabling hotkeys and creating scoped requests";
		dialogToShow->disableHotkeys(mHotkeyDisabler); // disable hotkeys while dialog open
		auto scopedRequests = getScopedRequests(); // pause game, free cursor, and block input while dialog open


		while (dialogToShow->isDialogOpen() == true)
		{
			if (GlobalKill::isKillSet()) // uh oh, program shutting down. Let's see if we have time to render one more frame to close the dialog properly.
			{
				dialogToShow->queueEmergencyClose();
				Sleep(30); // give time for one frame to render, hopefully
				currentOpenDialog = nullptr;
				return;

			}
			else
			{
				Sleep(10); // wait for dialog to close
			}

		}

		currentOpenDialog.reset();
		return;

	}

	ModalDialogRendererImpl(std::shared_ptr<RenderEvent> pRenderEvent, std::shared_ptr<ControlServiceContainer> controlServiceContainer, std::shared_ptr<GenericScopedServiceProvider> hotkeyDisabler)
		: mImGuiRenderCallbackHandle(pRenderEvent, [this](SimpleMath::Vector2 screenSize) {onImGuiRenderEvent(screenSize); }),
		mFreeMCCCursorService(controlServiceContainer->freeMCCSCursorService),
		mBlockGameInputService(controlServiceContainer->blockGameInputService),
		mPauseGameService(controlServiceContainer->pauseGameService),
		mHotkeyDisabler(hotkeyDisabler)
	{
	}
};


ModalDialogRenderer::ModalDialogRenderer(std::shared_ptr<RenderEvent> pRenderEvent, std::shared_ptr<ControlServiceContainer> controlServiceContainer, std::shared_ptr<GenericScopedServiceProvider> hotkeyDisabler)
	: pimpl(std::make_unique<ModalDialogRendererImpl>(pRenderEvent, controlServiceContainer, hotkeyDisabler))
	{}

ModalDialogRenderer::~ModalDialogRenderer() = default;




void ModalDialogRenderer::showVoidDialog(std::shared_ptr<IModalDialogVoid> dialogToShow)
{
	return pimpl->showVoidDialog(dialogToShow);
}

template <typename ret>
ret ModalDialogRenderer::showReturningDialog(std::shared_ptr<IModalDialogReturner<ret>>dialogToShow)
{
	return pimpl->showReturningDialog<ret>(dialogToShow);
}

// explicit template instantiations
template
bool ModalDialogRenderer::showReturningDialog(std::shared_ptr<IModalDialogReturner<bool>>dialogToShow);

template
std::tuple<bool, std::string> ModalDialogRenderer::showReturningDialog(std::shared_ptr<IModalDialogReturner<std::tuple<bool, std::string>>>dialogToShow);

template
std::optional<Waypoint> ModalDialogRenderer::showReturningDialog(std::shared_ptr<IModalDialogReturner<std::optional<Waypoint>>>dialogToShow);
