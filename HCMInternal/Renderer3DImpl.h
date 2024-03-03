#pragma once
#include "pch.h"
#include "DIContainer.h"
#include "IRenderer3D.h"
#include "GameState.h"
#include "IMessagesGUI.h"
#include "RuntimeExceptionHandler.h"
#include "IMCCStateHook.h"
#include "GetGameCameraData.h"
#include "IMakeOrGetCheat.h"
#include "SettingsStateAndEvents.h"
#include <d3d11.h>
#include "SpriteResource.h"
#include "directxtk\SpriteBatch.h"
#include "directxtk\DDSTextureLoader.h"
#include "directxtk\CommonStates.h"

// impl in .cpp
template<GameState::Value mGame>
class Renderer3DImpl : public IRenderer3D
{
private:
	// data
	SimpleMath::Vector3 cameraPosition;
	DirectX::SimpleMath::Matrix viewMatrix;
	DirectX::SimpleMath::Matrix projectionMatrix;
	SimpleMath::Vector2 screenSize;
	ID3D11Device* pDevice; 
	ID3D11DeviceContext* pDeviceContext; 
	ID3D11RenderTargetView* pMainRenderTargetView;
	bool init = false; // initialise spriteBatch & commonStates on first render frame
	std::unique_ptr<SpriteBatch> spriteBatch; // gets remade by constructSpriteResource to update sprite atlas
	std::unique_ptr<CommonStates> commonStates;


	// map of loaded resource IDs to their respective spriteResource.
	// If drawSprite is passed an ID not in this map, will call loadSpriteResource to try to load it and add to the map.
	std::map<int, std::unique_ptr<SpriteResource>> spriteResourceCache {}; // starts empty

	void constructSpriteResource(int resourceID); // throws HCMRuntimeExceptions on fail
	void initialise(); // run on first render frame to init spriteBatch & common states

	// injected services
	std::weak_ptr<GetGameCameraData> getGameCameraDataWeak;
	std::weak_ptr<SettingsStateAndEvents> settingsWeak;
	std::weak_ptr<IMCCStateHook> mccStateHookWeak;
	std::weak_ptr<IMessagesGUI> messagesGUIWeak;
	std::shared_ptr<RuntimeExceptionHandler> runtimeExceptions;

	// funcs
	virtual bool updateCameraData(ID3D11Device* pDevice, ID3D11DeviceContext* pDeviceContext, SimpleMath::Vector2 screenSize, ID3D11RenderTargetView* pMainRenderTargetView);
	RECTF drawSpriteImpl(int spriteResourceID, SimpleMath::Vector2 screenPosition, float spriteScale, bool shouldCenter);
	friend class Render3DEventProvider;
public:
	Renderer3DImpl(GameState game, IDIContainer& dicon);
	~Renderer3DImpl();

	virtual SimpleMath::Vector3 worldPointToScreenPosition(SimpleMath::Vector3 world) override;
	virtual float cameraDistanceToWorldPoint(SimpleMath::Vector3 worldPointPosition) override;
	virtual RECTF drawSprite(int spriteResourceID, SimpleMath::Vector2 screenPosition, float spriteScale) override;
	virtual RECTF drawCenteredSprite(int spriteResourceID, SimpleMath::Vector2 screenPosition, float spriteScale) override;


};