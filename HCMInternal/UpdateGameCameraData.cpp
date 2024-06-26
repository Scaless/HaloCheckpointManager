#include "pch.h"
#include "UpdateGameCameraData.h"

class UpdateGameCameraData::UpdateGameCameraDataImpl 
{
private:


public:
	UpdateGameCameraDataImpl(GameState gameImpl, IDIContainer& dicon)
	{

	}

	void updateGameCameraData(GameCameraData& gameCameraData, FreeCameraData& freeCameraData, float fovInDegrees)
	{
		// write camera data back to the game
		*gameCameraData.velocity = freeCameraData.currentPosition - *gameCameraData.position;
		*gameCameraData.position = freeCameraData.currentPosition;

		*gameCameraData.lookDirForward = freeCameraData.currentlookDirForward;
		*gameCameraData.lookDirUp = freeCameraData.currentlookDirUp;

		float fovInRadians = degreesToRadians(fovInDegrees); 

		*gameCameraData.FOV = fovInRadians;

	}
};



UpdateGameCameraData::UpdateGameCameraData(GameState gameImpl, IDIContainer& dicon)
{
	pimpl = std::make_unique< UpdateGameCameraDataImpl>(gameImpl, dicon);
}

UpdateGameCameraData::~UpdateGameCameraData()
{
	PLOG_DEBUG << "~" << getName();
}

void UpdateGameCameraData::updateGameCameraData(GameCameraData& gameCameraData, FreeCameraData& freeCameraData, float currentFOV)
{
	return pimpl->updateGameCameraData(gameCameraData, freeCameraData, currentFOV);
}