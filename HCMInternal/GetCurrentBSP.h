#pragma once
#include "pch.h"
#include "IOptionalCheat.h"
#include "DIContainer.h"
#include "GameState.h"


// actually returns zone set in case of h3+
class GetCurrentBSP : public IOptionalCheat
{
private:
	class GetCurrentBSPImpl;
	std::unique_ptr<GetCurrentBSPImpl> pimpl;

public:
	GetCurrentBSP(GameState game, IDIContainer& dicon);
	~GetCurrentBSP();

	DWORD getCurrentBSP();

	virtual std::string_view getName() override { return nameof(GetCurrentBSP); }
};

