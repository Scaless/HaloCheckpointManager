#pragma once
#include "pch.h"
#include "GameState.h"
#include "SettingsStateAndEvents.h"
#include "MultilevelPointer.h"
#include "PointerDataStore.h"
#include "IMessagesGUI.h"
#include "IOptionalCheat.h"
#include "DIContainer.h"
#include "RuntimeExceptionHandler.h"
#include "MultilevelPointer.h"
#include "InjectRequirements.h"
#include "ISharedMemory.h"
#include "IMCCStateHook.h"
#include "boost\iostreams\device\mapped_file.hpp"
#include "openssl\sha.h"
#include "GetCurrentLevelCode.h"
#include "ModalDialogRenderer.h"
#include "ModalDialogFactory.h"
#include "IMakeOrGetCheat.h"
#include "IGetMCCVersion.h"
#include "GetCurrentDifficulty.h"
#include "CheckpointInjectionLogger.h"
#include "DirPathContainer.h"

class InjectCheckpoint : public IOptionalCheat {
private:
	// which game is this implementation for
	GameState mImplGame; 

	// event callbacks
	ScopedCallback<ActionEvent> mInjectCheckpointEventCallback;

	// data
	std::shared_ptr<InjectRequirements> mInjectRequirements;
	std::shared_ptr<MultilevelPointer> mCheckpointLocation1;
	std::shared_ptr<MultilevelPointer> mCheckpointLocation2;
	std::shared_ptr<MultilevelPointer> mDoubleRevertFlag;
	std::shared_ptr<PreserveLocations> mPreserveLocations;
	std::shared_ptr<int64_t> mCheckpointLength;
	std::shared_ptr<offsetLengthPair> mSHAdata;
	std::shared_ptr<offsetLengthPair> mBSPdata;
	std::shared_ptr<MultilevelPointer> mLoadedBSP1;
	std::shared_ptr<MultilevelPointer> mLoadedBSP2;


	// injected services
	std::weak_ptr<IMCCStateHook> mccStateHookWeak;
	std::weak_ptr<IMessagesGUI> messagesGUIWeak;
	std::shared_ptr<RuntimeExceptionHandler> runtimeExceptions;
	std::weak_ptr<ISharedMemory> sharedMemWeak;
	std::weak_ptr<SettingsStateAndEvents> settingsWeak;
	std::weak_ptr<ModalDialogRenderer> modalDialogsWeak;
	std::weak_ptr<IGetMCCVersion> getMCCVerWeak;
	std::optional<std::weak_ptr<GetCurrentLevelCode>> levelCodeOptionalWeak;
	std::optional<std::weak_ptr<GetCurrentDifficulty>> difficultyOptionalWeak;

	CheckpointInjectionLogger checkpointInjectionLogger;

	void onInject() {

		try
		{

			lockOrThrow(mccStateHookWeak, mccStateHook);
			lockOrThrow(messagesGUIWeak, messagesGUI);
			lockOrThrow(getMCCVerWeak, getMCCVer);
			lockOrThrow(sharedMemWeak, sharedMem);
			lockOrThrow(modalDialogsWeak, modalDialogs);
			lockOrThrow(settingsWeak, settings);


			if (!mccStateHook->isGameCurrentlyPlaying(mImplGame)) return;
			PLOG_DEBUG << "Attempting checkpoint inject for game: " << mImplGame.toString();

			auto currentCheckpoint = sharedMem->getInjectInfo();

			if (currentCheckpoint.selectedCheckpointNull) throw HCMRuntimeException("Can't inject - no checkpoint selected!");
			if ((GameState)currentCheckpoint.selectedCheckpointGame != this->mImplGame) throw HCMRuntimeException(std::format("Can't inject - checkpoint from wrong game! Expected: {}, Actual: {}\n(Make sure you switch HCMExternal to the correct game tab)", mImplGame.toString(), ((GameState)currentCheckpoint.selectedCheckpointGame).toString()));
			

			//TODO: load correct level if level not aligned


			// check if user wants us to warn them on injecting to wrong level (and if we can do the check)
			if (settings->injectCheckpointLevelCheck->GetValue() && levelCodeOptionalWeak.has_value())
			{
				lockOrThrow(levelCodeOptionalWeak.value(), levelCode);
				try
				{
					// compare current level code to checkpoint level code (only check first 3 characters of each)
					if (levelCode->getCurrentLevelCode().substr(0, 3) != currentCheckpoint.selectedCheckpointLevelCode.substr(0, 3))
					{
						// no match! warn the user. This is a blocking call until they choose an option.

						auto continueWithInject = modalDialogs->showReturningDialog(ModalDialogFactory::makeInjectionWarningDialog("Injection: incorrect level warning!", std::format(
							"Warning! The checkpoint you are injecting appears to be from a different level than the one you are currently playing\n{}\nCheckpoint level: {}\nCurrent level: {}",
							"If this is the case, the game will probably crash. Continue anyway?",
							currentCheckpoint.selectedCheckpointLevelCode.substr(0, 3),
							levelCode->getCurrentLevelCode().substr(0, 3)
						)));

						if (!continueWithInject) return;
					}
				}
				catch (std::out_of_range ex)
				{
					PLOG_ERROR << "out_of_range exception occured testing checkpoint levelCode strings: " << std::endl
						<< "currentCheckpoint.selectedCheckpointLevelCode.size(): " << currentCheckpoint.selectedCheckpointLevelCode.size() << std::endl
						<< "levelCode.value()->getCurrentLevelCode().size(): " << levelCode->getCurrentLevelCode().size();
				}
			}

			// check if user wants us to warn them on injecting to wrong game version
			if (settings->injectCheckpointVersionCheck->GetValue())
			{
				// check if wrong game version
				if (getMCCVer->getMCCVersionAsString() != currentCheckpoint.selectedCheckpointGameVersion && currentCheckpoint.selectedCheckpointGameVersion.size() == 10)
				{
					// no match! warn the user. This is a blocking call until they choose an option.
					auto continueWithInject = modalDialogs->showReturningDialog(ModalDialogFactory::makeInjectionWarningDialog("Injection: incorrect game version warning!", std::format(
						"Warning! The checkpoint you are injecting appears to be from a different version of MCC than the one you are currently playing\n{}\nCheckpoint MCC ver: {}\nCurrent MCC ver: {}",
						"Checkpoints from different versions sometimes are, and sometimes aren't, compatible with eachother. Continue anyway?",
						currentCheckpoint.selectedCheckpointGameVersion,
						getMCCVer->getMCCVersionAsString()
					)));

					if (!continueWithInject) return;
				}
			}

			// check if user wants us to warn them on injecting to wrong difficulty
			if (settings->injectCheckpointDifficultyCheck->GetValue() && difficultyOptionalWeak.has_value() && magic_enum::enum_contains<DifficultyEnum>(currentCheckpoint.selectedCheckpointDifficulty))
			{
				lockOrThrow(difficultyOptionalWeak.value(), difficulty);

				// check if wrong game version
				if (difficulty->getCurrentDifficulty() != (DifficultyEnum)currentCheckpoint.selectedCheckpointDifficulty)
				{
					// no match! warn the user. This is a blocking call until they choose an option.
					auto continueWithInject = modalDialogs->showReturningDialog(ModalDialogFactory::makeInjectionWarningDialog("Injection: mismatched difficulty warning!", std::format(
						"Warning! The checkpoint you are injecting appears to have been played on a different difficulty than the one you are currently playing\n{}\nCheckpoint Difficulty: {}\nCurrent Difficulty: {}",
						"Injecting a checkpoint from a different difficulty can sometimes crash the game, and always messes up your pause menu interface. \nContinue anyway?",
						magic_enum::enum_name((DifficultyEnum)currentCheckpoint.selectedCheckpointDifficulty),
						magic_enum::enum_name(difficulty->getCurrentDifficulty())
					)));

					if (!continueWithInject) return;
				}
			}




			PLOG_DEBUG << "injectPath: " << currentCheckpoint.selectedCheckpointFilePath;

			// get checkpoint length (useful later)
			auto checkpointLength = *mCheckpointLength.get();

			// get the path to the checkpoint file to inject


			// check that the file actually exists
			if (!fileExists(currentCheckpoint.selectedCheckpointFilePath)) throw HCMRuntimeException(std::format("Checkpoint didn't exist at path: {}", currentCheckpoint.selectedCheckpointFilePath));;

			// check that the file is the correct length
			std::ifstream fileTestLength(currentCheckpoint.selectedCheckpointFilePath, std::ios::binary);
			fileTestLength.seekg(0, std::ios::end);
			uint64_t actualLength = fileTestLength.tellg();
			fileTestLength.close();
			if (actualLength != checkpointLength) throw HCMRuntimeException(std::format("Checkpoint was the wrong length, actual: 0x{:X}, expected: 0x{:X}", actualLength, checkpointLength));



			// get pointer to checkpoint in memory
			uintptr_t checkpointLoc = 0;
			if (mInjectRequirements->singleCheckpoint)
			{
				mCheckpointLocation1->resolve(&checkpointLoc);
				PLOG_DEBUG << "using checkpoint location @0x" << std::hex << (uint64_t)checkpointLoc;
			}
			else
			{
				bool firstCheckpoint;
				mDoubleRevertFlag->readData(&firstCheckpoint);
				PLOG_DEBUG << "Double revert flag value: " << firstCheckpoint;
				firstCheckpoint ? mCheckpointLocation1->resolve(&checkpointLoc) : mCheckpointLocation2->resolve(&checkpointLoc);
				PLOG_DEBUG << "using checkpoint location " << (firstCheckpoint ? "A" : "B") << " @0x" << std::hex << (uint64_t)checkpointLoc;
			}

			// check that the pointer is good
			if (IsBadWritePtr((void*)checkpointLoc, checkpointLength)) throw HCMRuntimeException(std::format("Bad inject read at 0x{:X}, length 0x{:X}", checkpointLoc, checkpointLength));


			// store preserveLocations of original checkpoint
			if (mInjectRequirements->preserveLocations)
			{
				PLOG_VERBOSE << "storing preserveLocations";
				for (auto& [offset, vec] : mPreserveLocations->locations)
				{
					auto err = memcpy_s(vec.data(), vec.size(), (void*)(checkpointLoc + offset), vec.size());
					if (err) throw HCMRuntimeException(std::format("error storing preserveLocation data! code: {}", err));
				}
			}

			// store checkpoint data in a vector buffer
			// uses memory-mapped file + memcpy
			boost::iostreams::mapped_file_source checkpointFile(currentCheckpoint.selectedCheckpointFilePath);
			if (!checkpointFile.is_open()) throw HCMRuntimeException(std::format("Failed to open checkpoint file for reading"));
			if (checkpointFile.size() != checkpointLength)
			{
				checkpointFile.close();
				throw HCMRuntimeException(std::format("Checkpoint file was incorrect length! expected: 0x{:X}, actual: 0x{:X}", checkpointLength, checkpointFile.size()));
			}
			std::vector<byte> checkpointData;
			checkpointData.resize(checkpointLength);
			auto err = memcpy_s(checkpointData.data(), checkpointLength, checkpointFile.data(), checkpointLength);
			checkpointFile.close();
			if (err) throw HCMRuntimeException(std::format("error loading checkpointdata from file! code: {}", err));
			if (checkpointData.size() != checkpointLength) 	throw HCMRuntimeException(std::format("Checkpoint data was incorrect length! expected: 0x{:X}, actual: 0x{:X}", checkpointLength, checkpointData.size()));

			// load preserveLocations
			if (mInjectRequirements->preserveLocations)
			{
				PLOG_VERBOSE << "loading preserveLocations";
				for (auto& [offset, vec] : mPreserveLocations->locations)
				{
					auto err = memcpy_s((void*)(checkpointData.data() + offset), vec.size(), vec.data(), vec.size());
					if (err) throw HCMRuntimeException(std::format("error loading preserveLocation data! code: {}", err));
				}
			}

			// zero out last 10 bytes
			for (auto it = checkpointData.rbegin(); it != checkpointData.rbegin() + 10; ++it)
			{
				*it = 0;
			}


			// calculate and set SHA
			if (mInjectRequirements->SHA)
			{
				PLOG_VERBOSE << "setting SHA data";
				// zero out SHA bytes in checkpoint Data
				for (auto it = checkpointData.begin() + mSHAdata->offset; it <= checkpointData.begin() + mSHAdata->offset + mSHAdata->length; ++it)
				{
					*it = 0;
				}
				std::vector<byte> SHAout;
				SHAout.reserve(mSHAdata->length);

				SHA1(checkpointData.data(), checkpointLength, SHAout.data());		// compute sha

				// paste it back into checkpoint data
				auto err = memcpy_s(checkpointData.data() + mSHAdata->offset, mSHAdata->length, SHAout.data(), SHAout.size());
				if (err) throw HCMRuntimeException(std::format("error loading checkpointdata from file! code: {}", err));
			}

			// inject checkpoint data into game memory
			err = memcpy_s((void*)checkpointLoc, checkpointLength, checkpointData.data(), checkpointData.size());
			if (err) throw HCMRuntimeException(std::format("error writing checkpointdata to game! code: {}", err));

			PLOG_VERBOSE << "wrote checkpoint data to location: " << std::hex << (uint64_t)checkpointLoc;

			// set BSP data
			if (mInjectRequirements->BSP)
			{
				PLOG_VERBOSE << "setting BSP data";

				PLOG_VERBOSE << "mBSPdata->length " << mBSPdata->length;
				PLOG_VERBOSE << "mBSPdata->offset " << mBSPdata->offset;
				PLOG_VERBOSE << "checkpointLoc + mBSPdata->offset " << std::hex << (uintptr_t)(checkpointLoc + mBSPdata->offset);
				
				byte* rawCurrentBSPData = (byte*)(checkpointLoc + mBSPdata->offset);
				std::vector<byte> currentBSPData (rawCurrentBSPData, rawCurrentBSPData + mBSPdata->length);

				if (mInjectRequirements->singleCheckpoint)
				{
					mLoadedBSP1->writeArrayData(currentBSPData.data(), currentBSPData.size());
				}
				else
				{
					bool firstCheckpoint;
					mDoubleRevertFlag->readData(&firstCheckpoint);
					PLOG_VERBOSE << "firstCheckpoint? " << firstCheckpoint;

					auto loadedBSPToWrite = firstCheckpoint ? mLoadedBSP1 : mLoadedBSP2;

#if HCM_DEBUG
					// attempting to debug a H2 crash
					uintptr_t loadedBSPLoc;
					if (!loadedBSPToWrite->resolve(&loadedBSPLoc)) throw HCMRuntimeException(std::format("Failed to resolve BSP data: {}", MultilevelPointer::GetLastError()));
					PLOG_VERBOSE << "loadedBSPLoc: " << std::hex << loadedBSPLoc;
					byte currentLoadedBSP;
					if (!loadedBSPToWrite->readData(&currentLoadedBSP)) throw HCMRuntimeException(std::format("Failed to read BSP data: {}", MultilevelPointer::GetLastError()));
					PLOG_VERBOSE << "currentLoadedBSP " << currentLoadedBSP;
#endif
					if (!loadedBSPToWrite->writeArrayData(currentBSPData.data(), currentBSPData.size())) throw HCMRuntimeException(std::format("Failed to write BSP data: {}", MultilevelPointer::GetLastError()));
					PLOG_VERBOSE << "successfully set BSP data";
				}
			}


			messagesGUI->addMessage(std::format("Injected checkpoint {}.bin", currentCheckpoint.selectedCheckpointName));

			PLOG_INFO << "succesfully injected checkpoint from path: " << currentCheckpoint.selectedCheckpointFilePath << " to " << std::hex << (uint64_t)checkpointLoc;
			
			checkpointInjectionLogger.logInjection(currentCheckpoint);
			
			if (settings->injectCheckpointForcesRevert->GetValue())
			{
				settings->forceRevertEvent->operator()();
			}

			

		}
		catch (HCMRuntimeException ex)
		{
			PLOG_VERBOSE << "error!" << ex.what();
			runtimeExceptions->handleMessage(ex);
		}

	}
	public:
		InjectCheckpoint(GameState game, IDIContainer& dicon)
			: mImplGame(game),
			mInjectCheckpointEventCallback(dicon.Resolve<SettingsStateAndEvents>().lock()->injectCheckpointEvent, [this]() { onInject(); }),
			mccStateHookWeak(dicon.Resolve<IMCCStateHook>()),
			messagesGUIWeak(dicon.Resolve<IMessagesGUI>()), 
			runtimeExceptions(dicon.Resolve<RuntimeExceptionHandler>()), 
			sharedMemWeak(dicon.Resolve<ISharedMemory>()),
			settingsWeak(dicon.Resolve<SettingsStateAndEvents>()),
			modalDialogsWeak(dicon.Resolve<ModalDialogRenderer>()),
			getMCCVerWeak(dicon.Resolve<IGetMCCVersion>()),
			checkpointInjectionLogger(dicon.Resolve<DirPathContainer>().lock()->dirPath)
		{
		auto ptr = dicon.Resolve<PointerDataStore>().lock();

		mInjectRequirements = ptr->getData<std::shared_ptr<InjectRequirements>>("injectRequirements", game);
		mCheckpointLength = ptr->getData< std::shared_ptr<int64_t>>("checkpointLength", game);
		mCheckpointLocation1 = ptr->getData< std::shared_ptr<MultilevelPointer>>("checkpointLocation1", game);

		if (!mInjectRequirements->singleCheckpoint)
		{
			mCheckpointLocation2 = ptr->getData< std::shared_ptr<MultilevelPointer>>("checkpointLocation2", game);
			mDoubleRevertFlag = ptr->getData< std::shared_ptr<MultilevelPointer>>("doubleRevertFlag", game);
		}

		if (mInjectRequirements->preserveLocations)
		{
			mPreserveLocations = ptr->getData< std::shared_ptr<PreserveLocations>>("preserveLocations", game);
		}

		if (mInjectRequirements->SHA)
		{
			mSHAdata = ptr->getData< std::shared_ptr<offsetLengthPair>>("SHAdata", game);
		}

		if (mInjectRequirements->BSP)
		{
			mBSPdata = ptr->getData< std::shared_ptr<offsetLengthPair>>("BSPdata", game);
			mLoadedBSP1 = ptr->getData< std::shared_ptr<MultilevelPointer>>("loadedBSP1", game);
			if (!mInjectRequirements->singleCheckpoint)
			{
				mLoadedBSP2 = ptr->getData< std::shared_ptr<MultilevelPointer>>("loadedBSP2", game);
			}

		}

		try
		{
			levelCodeOptionalWeak = std::dynamic_pointer_cast<GetCurrentLevelCode>( dicon.Resolve<IMakeOrGetCheat>().lock()->getOrMakeCheat({game, OptionalCheatEnum::GetCurrentLevelCode}, dicon));
		}
		catch (HCMInitException ex)
		{
			PLOG_ERROR << "Inject checkpoint couldn't acquire GetCurrentLevelCode service: " << ex.what();
		}

		try
		{
			difficultyOptionalWeak = std::dynamic_pointer_cast<GetCurrentDifficulty>(dicon.Resolve<IMakeOrGetCheat>().lock()->getOrMakeCheat({game, OptionalCheatEnum::GetCurrentDifficulty}, dicon));
		}
		catch (HCMInitException ex)
		{
			PLOG_ERROR << "Inject checkpoint couldn't acquire GetCurrentDifficulty service: " << ex.what();
		}

	}

		~InjectCheckpoint()
		{
			PLOG_VERBOSE << "~" << getName();
		}


		virtual std::string_view getName() override { return nameof(InjectCheckpoint); }
};

