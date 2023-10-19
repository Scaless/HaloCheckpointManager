#include "pch.h"
#include "PauseGame.h"
#include "ModuleHook.h"
#include "MidhookContextInterpreter.h"




class PauseGame::PauseGameImpl : public IProvideScopedRequests
{
private:
	static inline PauseGameImpl* instance = nullptr;

	std::set<std::string> callersRequestingBlockedInput{};
	std::map<GameState, std::shared_ptr<ModuleMidHook>> pauseGameInputHook;
	std::map<GameState, std::shared_ptr< MidhookContextInterpreter>> pauseGameFunctionContext;


	template<GameState::Value game>
	static void pauseGameHookFunction(SafetyHookContext& ctx) 
	{
		enum class param
		{
			tickRateUnit
		};
		auto* ctxInterpreter = instance->pauseGameFunctionContext.at(game).get();

		*(byte*)ctxInterpreter->getParameterRef(ctx, (int)param::tickRateUnit) = 0;
	}

public:
	std::map<GameState, HCMInitException> serviceFailures;

	PauseGameImpl(std::shared_ptr<PointerManager> ptr)
	{
		if (instance) throw HCMInitException("Cannot have more than one PauseGameImpl");

		for (auto game : AllSupportedGames)
		{
			try
			{
				auto pauseGameFunction = ptr->getData<std::shared_ptr<MultilevelPointer>>(nameof(pauseGameFunction), game);
				auto functionContext = ptr->getData<std::shared_ptr<MidhookContextInterpreter>>(nameof(pauseGameFunctionContext), game);
				pauseGameFunctionContext.emplace(game, functionContext);

				std::shared_ptr<ModuleMidHook> midhook;
				switch (game) // runtime gamevalue to template gamevalue shennaigans
				{
				case GameState::Value::Halo1: midhook = ModuleMidHook::make(game.toModuleName(), pauseGameFunction, pauseGameHookFunction<GameState::Value::Halo1>, false); break;
				case GameState::Value::Halo2: midhook = ModuleMidHook::make(game.toModuleName(), pauseGameFunction, pauseGameHookFunction<GameState::Value::Halo2>, false); break;
				case GameState::Value::Halo3: midhook = ModuleMidHook::make(game.toModuleName(), pauseGameFunction, pauseGameHookFunction<GameState::Value::Halo3>, false); break;
				case GameState::Value::Halo3ODST: midhook = ModuleMidHook::make(game.toModuleName(), pauseGameFunction, pauseGameHookFunction<GameState::Value::Halo3ODST>, false); break;
				case GameState::Value::HaloReach: midhook = ModuleMidHook::make(game.toModuleName(), pauseGameFunction, pauseGameHookFunction<GameState::Value::HaloReach>, false); break;
				case GameState::Value::Halo4: midhook = ModuleMidHook::make(game.toModuleName(), pauseGameFunction, pauseGameHookFunction<GameState::Value::Halo4>, false); break;
				}
				pauseGameInputHook.emplace(game, midhook);
			}
			catch (HCMInitException ex)
			{
				serviceFailures.emplace(game, ex);
			}
		}

		if (pauseGameInputHook.empty())
		{
			std::stringstream oss;
			for (auto& [game, ex] : serviceFailures)
			{
				oss << game.toString() << ": " << ex.what() << std::endl;
			}
			throw HCMInitException(std::format("Could not construct any game implementations\n{}", oss.str()));
		}


		instance = this;
	}

	~PauseGameImpl()
	{
		PLOG_DEBUG << "~" << nameof(PauseGameImpl);

		instance = nullptr;
	}
	void requestService(std::string callerID)
	{
		callersRequestingBlockedInput.insert(callerID);
		if (callersRequestingBlockedInput.empty() == false)
		{
			for (auto& [game, midhook] : pauseGameInputHook)
			{
				midhook->setWantsToBeAttached(true);
			}
		}
	}

	void unrequestService(std::string callerID)
	{
		callersRequestingBlockedInput.erase(callerID);
		if (callersRequestingBlockedInput.empty() == true)
		{
			for (auto& [game, midhook] : pauseGameInputHook)
			{
				midhook->setWantsToBeAttached(false);
			}
		}
	}

};








PauseGame::PauseGame(std::shared_ptr<PointerManager> ptr)
	: pimpl(std::make_shared< PauseGameImpl>(ptr)) {}

PauseGame::~PauseGame() = default;

std::unique_ptr<ScopedServiceRequest> PauseGame::scopedRequest(std::string callerID) { return std::make_unique<ScopedServiceRequest>(pimpl, callerID); }

std::map<GameState, HCMInitException>& PauseGame::getServiceFailures() { return pimpl->serviceFailures; }