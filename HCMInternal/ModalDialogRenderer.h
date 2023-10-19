#pragma once
#include "Vec2.h"
#include "ControlServiceContainer.h"
#include "GUIServiceInfo.h"
#include "IModalDialogRenderer.h"



class ModalDialogRenderer : public IModalDialogRenderer
{
private:
	class ModalDialogRendererImpl;
	std::unique_ptr<ModalDialogRendererImpl> pimpl;

	ScopedCallback<ActionEvent> mShowGUIFailuresEventCallback;
	std::shared_ptr<GUIServiceInfo> mGUIFailures;

public:
	ModalDialogRenderer(std::shared_ptr<RenderEvent> pRenderEvent, std::shared_ptr<ControlServiceContainer> controlServiceContainer, std::shared_ptr<ActionEvent> showFailsEvent, std::shared_ptr<GUIServiceInfo> guiFailures);
	~ModalDialogRenderer();
	virtual std::tuple<bool, std::string> showSaveDumpNameDialog(std::string dialogTitle, std::string defaultName) override; // This call blocks
	virtual bool showInjectionWarningDialog(std::string dialogTitle, std::string dialogText) override; // This call blocks
	virtual void showFailedOptionalCheatServices() override; // This call blocks

};
