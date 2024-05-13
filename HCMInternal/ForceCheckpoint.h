#pragma once
#include "pch.h"
#include "IOptionalCheat.h"
#include "GameState.h"
#include "DIContainer.h"
#include "IMCCStateHook.h"
#include "MultilevelPointer.h"
#include "PointerDataStore.h"
#include "IMessagesGUI.h"
#include "SettingsStateAndEvents.h"
#include "RuntimeExceptionHandler.h"



class ForceCheckpoint : public IOptionalCheat
{
private:



	// which game is this implementation for
	GameState mGame;

	// event callbacks
	ScopedCallback<ActionEvent> mForceCheckpointCallbackHandle;

	// injected services
	std::weak_ptr<IMCCStateHook> mccStateHookWeak;
	std::weak_ptr<IMessagesGUI> messagesGUIWeak;
	std::shared_ptr<RuntimeExceptionHandler> runtimeExceptions;


	//data
	std::shared_ptr<MultilevelPointer> forceCheckpointFlag;

	// primary event callback
	void onForceCheckpoint()
	{

		try
		{
			lockOrThrow(mccStateHookWeak, mccStateHook);
			lockOrThrow(messagesGUIWeak, messagesGUI);
			if (mccStateHook->isGameCurrentlyPlaying(mGame) == false) return;
			PLOG_DEBUG << "Force checkpoint called for game: " << mGame.toString();

			byte enableFlag = 1;
			if (!forceCheckpointFlag->writeData(&enableFlag)) throw HCMRuntimeException(std::format("Failed to write checkpoint flag {}", MultilevelPointer::GetLastError()));
			messagesGUI->addMessage("Checkpoint forced.");
		}
		catch (HCMRuntimeException ex)
		{
			runtimeExceptions->handleMessage(ex);
		}

	}



public:

	ForceCheckpoint(GameState gameImpl, IDIContainer& dicon)
		: mGame(gameImpl), 
		mForceCheckpointCallbackHandle(dicon.Resolve<SettingsStateAndEvents>().lock()->forceCheckpointEvent, [this]() {onForceCheckpoint(); }),
		mccStateHookWeak(dicon.Resolve<IMCCStateHook>()),
		messagesGUIWeak(dicon.Resolve<IMessagesGUI>()), 
		runtimeExceptions(dicon.Resolve<RuntimeExceptionHandler>())
		
	{
		PLOG_VERBOSE << "constructing ForceCheckpoint OptionalCheat for game: " << mGame.toString();
		auto ptr = dicon.Resolve<PointerDataStore>().lock();
		forceCheckpointFlag = ptr->getData<std::shared_ptr<MultilevelPointer>>("forceCheckpointFlag", mGame);
	}

	virtual std::string_view getName() override {
		return nameof(ForceCheckpoint);
	}

	~ForceCheckpoint()
	{
		PLOG_VERBOSE << "~ForceCheckpoint";
	}

};
